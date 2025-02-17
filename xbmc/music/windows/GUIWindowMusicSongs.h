#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIWindowMusicBase.h"

class CGUIWindowMusicSongs : public CGUIWindowMusicBase, public IBackgroundLoaderObserver
{
public:
  CGUIWindowMusicSongs(void);
  virtual ~CGUIWindowMusicSongs(void);

  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction& action);

  void DoScan(const CStdString &strPath);
protected:
  virtual void OnItemLoaded(CFileItem* pItem) {};
  virtual bool GetDirectory(const CStdString &strDirectory, CFileItemList &items);
  virtual void UpdateButtons();
  virtual bool Update(const CStdString &strDirectory, bool updateFilterPath = true);
  virtual void OnPrepareFileItems(CFileItemList &items);
  virtual void GetContextButtons(CFileItemPtr& item, CContextButtons &buttons);
  virtual bool OnContextButton(CFileItemPtr& item, CONTEXT_BUTTON button);
  virtual void OnScan(int iItem);
  virtual void OnRemoveSource(CFileItemPtr& item);
  virtual CStdString GetStartFolder(const CStdString &dir);

  // new method
  virtual void PlayItem(CFileItemPtr& pItem);

  void DeleteRemoveableMediaDirectoryCache();

  CMusicThumbLoader m_thumbLoader;
};
