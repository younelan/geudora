
#include <cairo.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "graph.h"
#include "legacy_shim.h"
#include "mailbox.h"
#include <gtk/gtk.h>
#include <pango/pangocairo.h>

enum {
  kBorderWidth = 12,
  kMinYHt = 12,
  kXAxisHt = 20,
  kDotSize = 8,
  kLegendBoxHeight = 12,
  kLegendBoxWidth = 16,
  kLegendLineHt = 24,
  kLegendTextSpace = 4,
  kYAxisWidth = 25,
  kTickSize = 5
};

static void SetRgbColor(cairo_t *cr, RGBColor *c) {
  cairo_set_source_rgb(cr, c->red / 65535.0, c->green / 65535.0,
                       c->blue / 65535.0);
}

static void SetForeGrey(cairo_t *cr, int greyValue) {
  double g = greyValue / 65535.0;
  cairo_set_source_rgb(cr, g, g, g);
}

static void DrawPangoText(cairo_t *cr, int x, int y, unsigned char *pstr) {
  if (!pstr || pstr[0] == 0)
    return;
  char buf[256];
  int len = pstr[0];
  if (len > 255)
    len = 255;
  memcpy(buf, pstr + 1, len);
  buf[len] = '\0';

  PangoLayout *layout = pango_cairo_create_layout(cr);
  pango_layout_set_text(layout, buf, -1);
  PangoFontDescription *desc = pango_font_description_from_string("Sans 9");
  pango_layout_set_font_description(layout, desc);
  pango_font_description_free(desc);

  // adjust Y to account for Mac QuickDraw drawing text upwards from baseline
  cairo_move_to(cr, x, y - 10);
  pango_cairo_show_layout(cr, layout);
  g_object_unref(layout);
}

static int StringWidthPango(cairo_t *cr, unsigned char *pstr) {
  if (!pstr || pstr[0] == 0)
    return 0;
  char buf[256];
  int len = pstr[0];
  if (len > 255)
    len = 255;
  memcpy(buf, pstr + 1, len);
  buf[len] = '\0';

  PangoLayout *layout = pango_cairo_create_layout(cr);
  pango_layout_set_text(layout, buf, -1);
  PangoFontDescription *desc = pango_font_description_from_string("Sans 9");
  pango_layout_set_font_description(layout, desc);
  pango_font_description_free(desc);
  int w, h;
  pango_layout_get_pixel_size(layout, &w, &h);
  g_object_unref(layout);
  return w;
}

static unsigned long GetMaxDataValue(short seriesCount, SeriesInfo *series,
                                     short dataCount) {
  short seriesIdx;
  unsigned long max = 0;
  for (seriesIdx = 0; seriesIdx < seriesCount; seriesIdx++) {
    short i;
    unsigned long *pData;
    unsigned long seriesMax;
    seriesMax = 0;
    pData = series[seriesIdx].data;
    for (i = 0; i < dataCount; i++) {
      if (pData[i] > seriesMax)
        seriesMax = pData[i];
    }
    if (series[seriesIdx].divider) {
      seriesMax = (seriesMax + series[seriesIdx].divider - 1) /
                  series[seriesIdx].divider;
    }
    if (seriesMax > max)
      max = seriesMax;
  }
  return max;
}

static void GetYAxisInfo(unsigned long maxValue, short maxGrids, short *yUnits,
                         short *yGrids) {
  short tens = 1;
  short rawScale;
  short scale;
  unsigned long max = maxValue;
  if (maxValue < 2)
    maxValue = 2;
  while (max > 10 * maxGrids) {
    max /= 10;
    tens *= 10;
  }
  if (max)
    max--;
  rawScale = max / maxGrids;
  if (rawScale >= 5)
    scale = 10;
  else if (rawScale >= 2)
    scale = 5;
  else if (rawScale >= 1)
    scale = 2;
  else
    scale = 1;
  *yUnits = scale * tens;
  *yGrids = (maxValue + *yUnits - 1) / (*yUnits);
}

static short GetYValue(unsigned long value, Rect *rBounds, unsigned long yUnits,
                       unsigned long yGrids) {
  return rBounds->bottom -
         value * ((unsigned long)(rBounds->bottom - rBounds->top)) /
             (yUnits * yGrids);
}

static void DrawYGrids(cairo_t *cr, Rect *rBounds, short yUnits, short yGrids,
                       short decPlaces) {
  short y, grid, i;
  unsigned char s[32];
  char cDecPoint = 0;

  // cDecPoint = decPlaces ? GetIntlNumberPart(2) : 0; // tokDecPoint stub
  cDecPoint = '.';

  for (i = 0, grid = 0; i <= yGrids; i++, grid += yUnits) {
    y = GetYValue(grid, rBounds, yUnits, yGrids);
    cairo_move_to(cr, rBounds->left - kTickSize, y);
    cairo_line_to(cr, rBounds->right - 1, y);
    cairo_stroke(cr);

    sprintf((char *)s + 1, "%d", grid);
    s[0] = strlen((char *)s + 1);

    if (decPlaces && grid) {
      if (s[0] == 1) { // insert 0
        memmove(s + 2, s + 1, s[0]);
        s[1] = '0';
        s[0]++;
      }
      if (cDecPoint) { // insert decimal
        memmove(s + 2 + s[0] - decPlaces, s + 1 + s[0] - decPlaces, decPlaces);
        s[1 + s[0] - decPlaces] = cDecPoint;
        s[0]++;
      }
    }
    DrawPangoText(cr, rBounds->left - StringWidthPango(cr, s) - 12, y + 4, s);
  }
}

static void DrawXGrids(cairo_t *cr, Rect *rBounds, GraphData *data) {
  short x, i, lastX = 0, lastPen = 0, thisPen;
  short xWidth = rBounds->right - rBounds->left - 1;
  unsigned char *sPtr = NULL;
  short tickHeight;

  if (data->xLabels)
    sPtr = (unsigned char *)data->xLabels + sizeof(short);

  for (i = 0; i <= data->dataCount; i++) {
    x = rBounds->left + i * xWidth / data->dataCount;
    cairo_move_to(cr, x,
                  (i == 0 || i == data->dataCount) ? rBounds->top
                                                   : rBounds->bottom);

    tickHeight = kTickSize;
    if (data->largeTickInterval && (i % data->largeTickInterval == 0))
      tickHeight += data->largeTickInterval;
    else if (data->medTickInterval && (i % data->medTickInterval == 0))
      tickHeight += data->medTickInterval;

    cairo_line_to(cr, x, rBounds->bottom + tickHeight);
    cairo_stroke(cr);

    if (i && data->xLabels) {
      short sWd = StringWidthPango(cr, sPtr);
      if (sPtr[0]) {
        // Determine center or left positioning
        int isdigit_first = (sPtr[1] >= '0' && sPtr[1] <= '9');
        thisPen = (sWd < x - lastX || !isdigit_first)
                      ? (x + lastX - sWd + 2) / 2
                      : x - sWd + 1;
        if (thisPen > lastPen && sPtr[0]) {
          DrawPangoText(cr, thisPen, rBounds->bottom + kXAxisHt, sPtr);
          lastPen = thisPen + sWd + 1;
        }
      }
      sPtr += sPtr[0] + 1;
    }
    lastX = x;
  }
}

void DrawPieGraph(cairo_t *cr, GraphData *data, Rect *rBounds) {
  unsigned long sum = 0, total = 0;
  short i, dataCount;
  double startAngle, arcAngle;
  double cx = (rBounds->left + rBounds->right) / 2.0;
  double cy = (rBounds->top + rBounds->bottom) / 2.0;
  double radius = (rBounds->right - rBounds->left) / 2.0;

  for (i = 0; i < data->dataCount; i++)
    total += ((unsigned long *)data->series[0].data)[i];
  if (total == 0)
    return;

  dataCount = data->dataCount < data->series[0].dataCount
                  ? data->dataCount
                  : data->series[0].dataCount;
  for (i = 0; i < dataCount; i++) {
    SetRgbColor(cr, &data->series[i].color);
    startAngle = sum * (2 * G_PI) / total;
    if (i == dataCount - 1)
      arcAngle = (2 * G_PI) - startAngle;
    else
      arcAngle =
          ((unsigned long *)data->series[0].data)[i] * (2 * G_PI) / total;

    cairo_move_to(cr, cx, cy);
    cairo_arc(cr, cx, cy, radius, startAngle, startAngle + arcAngle);
    cairo_close_path(cr);
    cairo_fill(cr);
    sum += ((unsigned long *)data->series[0].data)[i];
  }
  SetForeGrey(cr, 0);
  cairo_arc(cr, cx, cy, radius, 0, 2 * G_PI);
  cairo_stroke(cr);
}

static void DrawLegend(cairo_t *cr, GraphData *data, Rect *rBounds, short top) {
  short seriesIdx;
  Rect rColor, rDot;

  for (seriesIdx = 0; seriesIdx < data->seriesCount; seriesIdx++) {
    rColor.left = rBounds->right + kBorderWidth;
    rColor.top = top;
    rColor.right = rColor.left + kLegendBoxWidth;
    rColor.bottom = top + kLegendBoxHeight;

    rDot.left = rColor.left + (kLegendBoxWidth - kDotSize) / 2;
    rDot.top = top + (kLegendBoxHeight - kDotSize) / 2;
    rDot.right = rDot.left + kDotSize;
    rDot.bottom = rDot.top + kDotSize;

    SetRgbColor(cr, &data->series[seriesIdx].color);
    GraphType type = data->series[seriesIdx].type ? data->series[seriesIdx].type
                                                  : data->type;

    switch (type) {
    case kBarGraph:
    case kPieGraph:
      cairo_rectangle(cr, rColor.left, rColor.top, kLegendBoxWidth,
                      kLegendBoxHeight);
      cairo_fill(cr);
      SetForeGrey(cr, 0);
      cairo_rectangle(cr, rColor.left, rColor.top, kLegendBoxWidth,
                      kLegendBoxHeight);
      cairo_stroke(cr);
      break;
    case kLineGraph:
      cairo_set_line_width(cr, 2.0);
      cairo_move_to(cr, rColor.left, rColor.top + kLegendBoxHeight / 2);
      cairo_line_to(cr, rColor.right - 1, rColor.top + kLegendBoxHeight / 2);
      cairo_stroke(cr);
      break;
    case kAreaGraph:
      cairo_move_to(cr, rColor.left, rColor.bottom);
      cairo_line_to(cr, rColor.left, rColor.bottom - 6);
      cairo_line_to(cr, rColor.left + kLegendBoxWidth / 4, rColor.bottom - 11);
      cairo_line_to(cr, rColor.left + kLegendBoxWidth / 2, rColor.bottom - 8);
      cairo_line_to(cr, rColor.left + 3 * kLegendBoxWidth / 4,
                    rColor.bottom - 12);
      cairo_line_to(cr, rColor.right, rColor.bottom - 7);
      cairo_line_to(cr, rColor.right, rColor.bottom);
      cairo_close_path(cr);
      cairo_fill(cr);
      SetForeGrey(cr, 0);
      cairo_move_to(cr, rColor.left, rColor.bottom);
      cairo_line_to(cr, rColor.left, rColor.bottom - 6);
      cairo_line_to(cr, rColor.left + kLegendBoxWidth / 4, rColor.bottom - 11);
      cairo_line_to(cr, rColor.left + kLegendBoxWidth / 2, rColor.bottom - 8);
      cairo_line_to(cr, rColor.left + 3 * kLegendBoxWidth / 4,
                    rColor.bottom - 12);
      cairo_line_to(cr, rColor.right, rColor.bottom - 7);
      cairo_line_to(cr, rColor.right, rColor.bottom);
      cairo_close_path(cr);
      cairo_stroke(cr);
      break;
    case kCircleGraph:
      cairo_arc(cr, (rDot.left + rDot.right) / 2.0,
                (rDot.top + rDot.bottom) / 2.0, kDotSize / 2.0, 0, 2 * G_PI);
      cairo_fill(cr);
      break;
    case kDiamondGraph:
      cairo_move_to(cr, rDot.left, rDot.top + kDotSize / 2.0);
      cairo_line_to(cr, rDot.left + kDotSize / 2.0, rDot.top);
      cairo_line_to(cr, rDot.right, rDot.top + kDotSize / 2.0);
      cairo_line_to(cr, rDot.left + kDotSize / 2.0, rDot.bottom);
      cairo_close_path(cr);
      cairo_fill(cr);
      break;
    case kSquareGraph:
      cairo_rectangle(cr, rDot.left, rDot.top, kDotSize, kDotSize);
      cairo_fill(cr);
      break;
    default:
      break;
    }

    SetForeGrey(cr, 0);
    DrawPangoText(cr, rColor.right + kLegendTextSpace, rColor.bottom - 3,
                  (unsigned char *)data->series[seriesIdx].label);
    top += kLegendLineHt;
  }
}

void DrawGraph(cairo_t *cr, GraphData *data) {
  unsigned long maxValue;
  Rect rBounds;
  unsigned char s[32];
  short yUnits, yGrids;
  short seriesIdx;
  GraphType theType;
  unsigned long divider = 1;
  short barCount, barIdx = 0, barWidth, width;
  short xWidth, barMargin, yLegend;

  cairo_set_line_width(cr, 1.0);
  cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE);

  rBounds = data->bounds;
  rBounds.top += kBorderWidth;
  rBounds.left += kBorderWidth;
  rBounds.bottom -= kBorderWidth;
  rBounds.right -= kBorderWidth;

  if (data->legend) {
    short maxLabelWidth = 0;
    for (seriesIdx = 0; seriesIdx < data->seriesCount; seriesIdx++) {
      width =
          StringWidthPango(cr, (unsigned char *)data->series[seriesIdx].label);
      if (width > maxLabelWidth)
        maxLabelWidth = width;
    }
    width = maxLabelWidth + kBorderWidth + kLegendTextSpace + kLegendBoxWidth;
    if (width < 30)
      width = 30; // STAT_LEGEND_WIDTH stub
    rBounds.right -= width;
  }

  yLegend = rBounds.top;

  // Always use seriesIdx 0 type as baseline check if unspecified
  theType = data->series[0].type ? data->series[0].type : data->type;

  if (data->type == kPieGraph || data->series[0].type == kPieGraph) {
    short size = rBounds.bottom - rBounds.top;
    short wi = rBounds.right - rBounds.left;
    size = size < wi ? size : wi;
    rBounds.right = rBounds.left + size;
    rBounds.bottom = rBounds.top + size;
    DrawPieGraph(cr, data, &rBounds);
  } else {
    rBounds.bottom -= kXAxisHt;

    maxValue =
        GetMaxDataValue(data->seriesCount, data->series, data->dataCount);
    if (data->divider) {
      maxValue = (maxValue + data->divider - 1) / data->divider;
      divider = data->divider;
    }

    sprintf((char *)s + 1, "%lu", (unsigned long)maxValue);
    s[0] = strlen((char *)s + 1);

    if (data->decPlace) {
      char cDecPoint = '.';
      s[s[0] + 1] = cDecPoint;
      s[0]++;
      for (int decCount = data->decPlace; decCount; decCount--) {
        s[s[0] + 1] = '0';
        s[0]++;
      }
    }

    width = StringWidthPango(cr, s) + 8;
    if (width < kYAxisWidth)
      width = kYAxisWidth;
    rBounds.left += width;

    GetYAxisInfo(maxValue, (rBounds.bottom - rBounds.top) / kMinYHt, &yUnits,
                 &yGrids);

    SetForeGrey(cr, 49344);
    cairo_rectangle(cr, rBounds.left, rBounds.top, rBounds.right - rBounds.left,
                    rBounds.bottom - rBounds.top);
    cairo_fill(cr);

    SetForeGrey(cr, 0);
    DrawYGrids(cr, &rBounds, yUnits, yGrids, data->decPlace);
    DrawXGrids(cr, &rBounds, data);

    xWidth = rBounds.right - rBounds.left;

    barCount = 0;
    for (seriesIdx = 0; seriesIdx < data->seriesCount; seriesIdx++) {
      if ((data->series[seriesIdx].type ? data->series[seriesIdx].type
                                        : data->type) == kBarGraph)
        barCount++;
    }

    if (barCount) {
      barWidth = xWidth / data->dataCount;
      barMargin = barWidth / 8;
      barWidth -= barMargin * 2;
      barWidth /= barCount;
      if (!barWidth)
        barWidth = 1;
    }

    if (data->decPlace && divider > 10 && yUnits < 10) {
      yUnits *= 10;
      divider /= 10;
    }

    for (seriesIdx = 0; seriesIdx < data->seriesCount; seriesIdx++) {
      short i;
      unsigned long *pData = (unsigned long *)data->series[seriesIdx].data;
      short x1, x2, x, y, firstX = 0;
      unsigned long dataValue, lastData = 0;
      short dataCount = data->dataCount < data->series[seriesIdx].dataCount
                            ? data->dataCount
                            : data->series[seriesIdx].dataCount;
      unsigned long useYUnits;

      theType = data->series[seriesIdx].type ? data->series[seriesIdx].type
                                             : data->type;
      if (theType == kLineGraph)
        cairo_set_line_width(cr, 2.0);
      else
        cairo_set_line_width(cr, 1.0);

      x1 = rBounds.left;
      SetRgbColor(cr, &data->series[seriesIdx].color);

      for (i = 0; i < dataCount; i++) {
        dataValue = pData[i];
        useYUnits = yUnits;
        if (divider != 1)
          useYUnits *= divider;
        if (data->series[seriesIdx].divider)
          useYUnits *= data->series[seriesIdx].divider;

        y = GetYValue(dataValue, &rBounds, useYUnits, yGrids);
        x2 = rBounds.left + (i + 1) * xWidth / data->dataCount;
        x = (x1 + x2) / 2;

        switch (theType) {
        case kBarGraph:
          if (dataValue) {
            short xLeft = x1 + barMargin + barIdx * barWidth;
            cairo_rectangle(cr, xLeft, y, barWidth, rBounds.bottom - y + 1);
            cairo_fill(cr);
            SetForeGrey(cr, 0);
            if (barWidth > 3) {
              cairo_rectangle(cr, xLeft, y, barWidth, rBounds.bottom - y + 1);
              cairo_stroke(cr);
            }
            SetRgbColor(cr, &data->series[seriesIdx].color);
          }
          break;
        case kLineGraph:
          if (i && (lastData || dataValue))
            cairo_line_to(cr, x, y);
          else {
            if (dataCount > 1)
              cairo_move_to(cr, x, y);
            else {
              cairo_move_to(cr, x1, y);
              cairo_line_to(cr, x, y);
            }
          }
          lastData = dataValue;
          break;
        case kAreaGraph:
          if (i)
            cairo_line_to(cr, x, y);
          else {
            firstX = x;
            if (dataCount > 1)
              cairo_move_to(cr, x, y);
            else {
              cairo_move_to(cr, x1, y);
              cairo_line_to(cr, x, y);
              firstX = x1;
            }
          }
          break;
        case kCircleGraph:
          if (dataValue) {
            cairo_arc(cr, x, y, kDotSize / 2.0, 0, 2 * G_PI);
            cairo_fill(cr);
          }
          break;
        case kDiamondGraph:
          if (dataValue) {
            cairo_move_to(cr, x - kDotSize / 2.0, y);
            cairo_line_to(cr, x, y - kDotSize / 2.0);
            cairo_line_to(cr, x + kDotSize / 2.0, y);
            cairo_line_to(cr, x, y + kDotSize / 2.0);
            cairo_close_path(cr);
            cairo_fill(cr);
          }
          break;
        case kSquareGraph:
          if (dataValue) {
            cairo_rectangle(cr, x - kDotSize / 2.0, y - kDotSize / 2.0,
                            kDotSize, kDotSize);
            cairo_fill(cr);
          }
          break;
        default:
          break;
        }
        x1 = x2;
      }
      if (theType == kBarGraph)
        barIdx++;
      else if (theType == kAreaGraph) {
        cairo_line_to(cr, x, rBounds.bottom);
        cairo_line_to(cr, firstX, rBounds.bottom);
        cairo_close_path(cr);
        cairo_fill(cr);
      } else if (theType == kLineGraph) {
        cairo_stroke(cr); // commit the path
      }
    }
  }

  if (data->legend) {
    DrawLegend(cr, data, &rBounds, yLegend);
  }
}
