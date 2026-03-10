// ResourceManager.h
#pragma once

#include <string>
#include <vector>
#include <wx/bitmap.h>
#include <wx/cursor.h>
#include <wx/dialog.h>
#include <wx/icon.h>
#include <wx/menu.h>
#include <wx/string.h>
#include <wx/window.h>

class CPalette;

class ResourceManager {
public:
  static ResourceManager &Get();

  // Initialize resources (calls InitResources() from generated map)
  void Init();

  // Get a bitmap by symbolic id (e.g. "IDR_MAINFRAME32")
  wxBitmap GetBitmap(const std::string &id) const;

  // Get a localized string by numeric resource id
  wxString GetString(unsigned int id) const;

  // Get a localized string by symbolic name (looks up XRC ID first)
  wxString GetString(const std::string &name) const;

  // Get a numeric XRC ID from a symbolic name
  int GetXRCID(const std::string &name) const;

  // Load an XRC file relative to the resources path (e.g. "main.xrc")
  bool LoadXRC(const std::string &xrcFilename) const;

  // Load a menubar by its XRC name; returns nullptr if not found
  wxMenuBar *LoadMenuBar(const std::string &name) const;

  // Load a menu by its XRC name; returns a new wxMenu* (caller owns) or
  // nullptr if not found.
  wxMenu *LoadMenu(const std::string &name) const;

  // Load an icon by symbolic name or numeric id. Returns an empty wxIcon on
  // failure.
  wxIcon LoadIcon(const std::string &name) const;
  wxIcon LoadIcon(unsigned int id) const;

  // Load a cursor by symbolic name or numeric id.
  wxCursor LoadCursor(const std::string &name) const;
  wxCursor LoadCursor(unsigned int id) const;

  // Load a dialog by symbolic name or numeric id.
  // Returns a new wxDialog* (caller owns) or nullptr if not found.
  // The dialog is created but NOT shown - caller should call ShowModal() or
  // Show().
  wxDialog *LoadDialog(wxWindow *parent, const std::string &name) const;
  wxDialog *LoadDialog(wxWindow *parent, unsigned int id) const;

  // Load a panel into an existing instance
  bool LoadPanel(wxPanel *panel, const std::string &name) const;

  // Load a dialog into an existing instance (e.g. for subclasses)
  // Returns true on success.
  bool LoadDialog(wxDialog *dlg, wxWindow *parent,
                  const std::string &name) const;

  // Load a device-independent bitmap (portable): prefer generated PNG/XRC
  // resources and fall back to the original Windows DIB resource path when
  // running on Windows. `CPalette` is honored on Windows builds only.
  wxBitmap LoadDIBitmap(const char *lpString, CPalette *pPalette) const;

  // Slice a toolbar bitmap into tiles (returns empty vector on failure)
  std::vector<wxBitmap> GetToolbarTiles(const std::string &id) const;

private:
  ResourceManager();
  ~ResourceManager();
  // non-copyable
  ResourceManager(const ResourceManager &) = delete;
  ResourceManager &operator=(const ResourceManager &) = delete;
};
