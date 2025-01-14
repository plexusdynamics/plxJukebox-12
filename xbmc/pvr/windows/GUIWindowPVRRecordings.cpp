/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include "GUIWindowPVRRecordings.h"

#include "guilib/GUIKeyboardFactory.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "GUIInfoManager.h"
#include "pvr/PVRManager.h"
#include "pvr/recordings/PVRRecordings.h"
#include "pvr/timers/PVRTimers.h"
#include "pvr/windows/GUIWindowPVR.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "threads/SingleLock.h"
#include "pvr/addons/PVRClients.h"
#include "video/windows/GUIWindowVideoNav.h"

using namespace PVR;

CGUIWindowPVRRecordings::CGUIWindowPVRRecordings(CGUIWindowPVR *parent) :
  CGUIWindowPVRCommon(parent, PVR_WINDOW_RECORDINGS, CONTROL_BTNRECORDINGS, CONTROL_LIST_RECORDINGS)
{
  m_strSelectedPath = "pvr://recordings/";
  m_thumbLoader.SetNumOfWorkers(1);
}

void CGUIWindowPVRRecordings::UnregisterObservers(void)
{
  CSingleLock lock(m_critSection);
  if(g_PVRRecordings)
    g_PVRRecordings->UnregisterObserver(this);
  if(g_PVRTimers)
    g_PVRTimers->UnregisterObserver(this);
  g_infoManager.UnregisterObserver(this);
}

void CGUIWindowPVRRecordings::ResetObservers(void)
{
  CSingleLock lock(m_critSection);
  g_PVRRecordings->RegisterObserver(this);
  g_PVRTimers->RegisterObserver(this);
  g_infoManager.RegisterObserver(this);
}

CStdString CGUIWindowPVRRecordings::GetResumeString(const CFileItem& item)
{
  CStdString resumeString;
  if (item.IsPVRRecording())
  {

    // First try to find the resume position on the back-end, if that fails use video database
    int positionInSeconds = item.GetPVRRecordingInfoTag()->GetLastPlayedPosition();
    // If the back-end does report a saved position then make sure there is a corresponding resume bookmark
    if (positionInSeconds > 0)
    {
      CBookmark bookmark;
      bookmark.timeInSeconds = positionInSeconds;
      bookmark.totalTimeInSeconds = (double)item.GetPVRRecordingInfoTag()->GetDuration();
      CVideoDatabase db;
      if (db.Open())
      {
        CStdString itemPath(item.GetPVRRecordingInfoTag()->m_strFileNameAndPath);
        db.AddBookMarkToFile(itemPath, bookmark, CBookmark::RESUME);
        db.Close();
      }
    }
    else if (positionInSeconds < 0)
    {
      CVideoDatabase db;
      if (db.Open())
      {
        CBookmark bookmark;
        CStdString itemPath(item.GetPVRRecordingInfoTag()->m_strFileNameAndPath);
        if (db.GetResumeBookMark(itemPath, bookmark) )
          positionInSeconds = lrint(bookmark.timeInSeconds);
        db.Close();
      }
    }

    // Suppress resume from 0
    if (positionInSeconds > 0)
      resumeString.Format(g_localizeStrings.Get(12022).c_str(), StringUtils::SecondsToTimeString(positionInSeconds).c_str());
  }
  return resumeString;
}

void CGUIWindowPVRRecordings::GetContextButtons(int itemNumber, CContextButtons &buttons) const
{
  if (itemNumber < 0 || itemNumber >= m_parent->m_vecItems->Size())
    return;
  CFileItemPtr pItem = m_parent->m_vecItems->Get(itemNumber);

  if (pItem->HasPVRRecordingInfoTag())
  {
    buttons.Add(CONTEXT_BUTTON_INFO, 19053);      /* Get Information of this recording */
    buttons.Add(CONTEXT_BUTTON_FIND, 19003);      /* Find similar program */
    buttons.Add(CONTEXT_BUTTON_PLAY_ITEM, 12021); /* Play this recording */
    CStdString resumeString = GetResumeString(*pItem);
    if (!resumeString.IsEmpty())
    {
      buttons.Add(CONTEXT_BUTTON_RESUME_ITEM, resumeString);
    }
  }
  if (pItem->m_bIsFolder)
  {
    // Have both options for folders since we don't know whether all childs are watched/unwatched
    buttons.Add(CONTEXT_BUTTON_MARK_UNWATCHED, 16104); /* Mark as UnWatched */
    buttons.Add(CONTEXT_BUTTON_MARK_WATCHED, 16103);   /* Mark as Watched */
  }
  if (pItem->HasPVRRecordingInfoTag())
  {
    if (pItem->GetPVRRecordingInfoTag()->m_playCount > 0)
      buttons.Add(CONTEXT_BUTTON_MARK_UNWATCHED, 16104); /* Mark as UnWatched */
    else
      buttons.Add(CONTEXT_BUTTON_MARK_WATCHED, 16103);   /* Mark as Watched */

    buttons.Add(CONTEXT_BUTTON_RENAME, 118);      /* Rename this recording */
    buttons.Add(CONTEXT_BUTTON_DELETE, 117);      /* Delete this recording */
  }
  buttons.Add(CONTEXT_BUTTON_SORTBY_NAME, 103);       /* sort by name */
  buttons.Add(CONTEXT_BUTTON_SORTBY_DATE, 104);       /* sort by date */

  if (pItem->HasPVRRecordingInfoTag() &&
      g_PVRClients->HasMenuHooks(pItem->GetPVRRecordingInfoTag()->m_iClientId, PVR_MENUHOOK_RECORDING))
    buttons.Add(CONTEXT_BUTTON_MENU_HOOKS, 19195);      /* PVR client specific action */

  // Update sort by button
//if (m_guiState->GetSortMethod()!=SORT_METHOD_NONE)
//{
//  CStdString sortLabel;
//  sortLabel.Format(g_localizeStrings.Get(550).c_str(), g_localizeStrings.Get(m_guiState->GetSortMethodLabel()).c_str());
//  buttons.Add(CONTEXT_BUTTON_SORTBY, sortLabel);   /* Sort method */
//
//  if (m_guiState->GetDisplaySortOrder()==SORT_ORDER_ASC)
//    buttons.Add(CONTEXT_BUTTON_SORTASC, 584);        /* Sort up or down */
//  else
//    buttons.Add(CONTEXT_BUTTON_SORTASC, 585);        /* Sort up or down */
//}
}

bool CGUIWindowPVRRecordings::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_PARENT_DIR ||
      action.GetID() == ACTION_NAV_BACK)
  {
    if (m_parent->m_vecItems->GetPath() != "pvr://recordings/")
      m_parent->GoParentFolder();
    else if (action.GetID() == ACTION_NAV_BACK)
      g_windowManager.PreviousWindow();

    return true;
  }

  return CGUIWindowPVRCommon::OnAction(action);
}

bool CGUIWindowPVRRecordings::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  if (itemNumber < 0 || itemNumber >= m_parent->m_vecItems->Size())
    return false;
  CFileItemPtr pItem = m_parent->m_vecItems->Get(itemNumber);

  return OnContextButtonPlay(pItem.get(), button) ||
      OnContextButtonRename(pItem.get(), button) ||
      OnContextButtonDelete(pItem.get(), button) ||
      OnContextButtonInfo(pItem.get(), button) ||
      OnContextButtonMarkWatched(pItem, button) ||
      CGUIWindowPVRCommon::OnContextButton(itemNumber, button);
}

void CGUIWindowPVRRecordings::OnWindowUnload(void)
{
  m_strSelectedPath = m_parent->m_vecItems->GetPath();
  CGUIWindowPVRCommon::OnWindowUnload();
}

void CGUIWindowPVRRecordings::UpdateData(bool bUpdateSelectedFile /* = true */)
{
  CSingleLock lock(m_critSection);
  CLog::Log(LOGDEBUG, "CGUIWindowPVRRecordings - %s - update window '%s'. set view to %d", __FUNCTION__, GetName(), m_iControlList);
  m_bUpdateRequired = false;

  /* lock the graphics context while updating */
  CSingleLock graphicsLock(g_graphicsContext);

  m_iSelected = m_parent->m_viewControl.GetSelectedItem();
  if (m_parent->m_vecItems->GetPath().Left(17) != "pvr://recordings/")
    m_strSelectedPath = "pvr://recordings/";
  else
    m_strSelectedPath = m_parent->m_vecItems->GetPath();

  m_parent->m_viewControl.SetCurrentView(m_iControlList);
  ShowBusyItem();
  m_parent->m_vecItems->Clear();
  m_parent->m_vecItems->SetPath(m_strSelectedPath);
  m_parent->Update(m_strSelectedPath);
  m_parent->m_viewControl.SetItems(*m_parent->m_vecItems);

  if (bUpdateSelectedFile)
  {
    if (!SelectPlayingFile())
      m_parent->m_viewControl.SetSelectedItem(m_iSelected);
  }

  m_parent->SetLabel(CONTROL_LABELHEADER, g_localizeStrings.Get(19017));
  m_parent->SetLabel(CONTROL_LABELGROUP, "");
}

void CGUIWindowPVRRecordings::Notify(const Observable &obs, const ObservableMessage msg)
{
  if (msg == ObservableMessageTimers || msg == ObservableMessageCurrentItem)
  {
    if (IsVisible())
      SetInvalid();
    else
      m_bUpdateRequired = true;
  }
  else if (msg == ObservableMessageRecordings || msg == ObservableMessageTimersReset)
  {
    if (IsVisible())
      UpdateData();
    else
      m_bUpdateRequired = true;
  }
}

bool CGUIWindowPVRRecordings::OnClickButton(CGUIMessage &message)
{
  bool bReturn = false;

  if (IsSelectedButton(message))
  {
    bReturn = true;
    g_PVRManager.TriggerRecordingsUpdate();
  }

  return bReturn;
}

bool CGUIWindowPVRRecordings::OnClickList(CGUIMessage &message)
{
  bool bReturn = false;

  if (IsSelectedList(message))
  {
    bReturn = true;
    int iAction = message.GetParam1();
    int iItem = m_parent->m_viewControl.GetSelectedItem();

    /* get the fileitem pointer */
    if (iItem < 0 || iItem >= (int) m_parent->m_vecItems->Size())
      return bReturn;
    CFileItemPtr pItem = m_parent->m_vecItems->Get(iItem);

    /* process actions */
    if (iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK || iAction == ACTION_PLAY)
    {
      int choice = CONTEXT_BUTTON_PLAY_ITEM;
      CStdString resumeString = GetResumeString(*pItem);
      if (!resumeString.IsEmpty())
      {
        CContextButtons choices;
        choices.Add(CONTEXT_BUTTON_RESUME_ITEM, resumeString);
        choices.Add(CONTEXT_BUTTON_PLAY_ITEM, 12021);
        choice = CGUIDialogContextMenu::ShowAndGetChoice(choices);
      }
      if (choice < 0)
        bReturn = true;
      else
        bReturn = OnContextButtonPlay(pItem.get(), (CONTEXT_BUTTON)choice);
    }
    else if (iAction == ACTION_CONTEXT_MENU || iAction == ACTION_MOUSE_RIGHT_CLICK)
      m_parent->OnPopupMenu(pItem);
    else if (iAction == ACTION_SHOW_INFO)
      ShowRecordingInfo(pItem.get());
    else if (iAction == ACTION_DELETE_ITEM)
      bReturn = ActionDeleteRecording(pItem.get());
    else
      bReturn = false;
  }

  return bReturn;
}

bool CGUIWindowPVRRecordings::OnContextButtonDelete(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_DELETE)
  {
    bReturn = false;

    CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
    if (!pDialog)
      return bReturn;
    pDialog->SetHeading(122);
    pDialog->SetLine(0, 19043);
    pDialog->SetLine(1, "");
    pDialog->SetLine(2, item->GetPVRRecordingInfoTag()->m_strTitle);
    pDialog->DoModal();

    if (!pDialog->IsConfirmed())
      return bReturn;

    bReturn = g_PVRRecordings->DeleteRecording(*item);
  }

  return bReturn;
}

bool CGUIWindowPVRRecordings::OnContextButtonInfo(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_INFO)
  {
    bReturn = true;
    ShowRecordingInfo(item);
  }

  return bReturn;
}

bool CGUIWindowPVRRecordings::OnContextButtonPlay(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if ((button == CONTEXT_BUTTON_PLAY_ITEM) ||
      (button == CONTEXT_BUTTON_RESUME_ITEM))
  {
    item->m_lStartOffset = button == CONTEXT_BUTTON_RESUME_ITEM ? STARTOFFSET_RESUME : 0;
    bReturn = PlayFile(item, false); /* play recording */
  }

  return bReturn;
}

bool CGUIWindowPVRRecordings::OnContextButtonRename(CFileItem *item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_RENAME)
  {
    bReturn = true;

    CPVRRecording *recording = item->GetPVRRecordingInfoTag();
    CStdString strNewName = recording->m_strTitle;
    if (CGUIKeyboardFactory::ShowAndGetInput(strNewName, g_localizeStrings.Get(19041), false))
    {
      if (g_PVRRecordings->RenameRecording(*item, strNewName))
        UpdateData();
    }
  }

  return bReturn;
}

bool CGUIWindowPVRRecordings::OnContextButtonMarkWatched(const CFileItemPtr &item, CONTEXT_BUTTON button)
{
  bool bReturn = false;

  if (button == CONTEXT_BUTTON_MARK_WATCHED)
  {
    bReturn = true;

    int newSelection = m_parent->m_viewControl.GetSelectedItem();
    g_PVRRecordings->SetRecordingsPlayCount(item, 1);
    m_parent->m_viewControl.SetSelectedItem(newSelection);

    UpdateData();
  }

  if (button == CONTEXT_BUTTON_MARK_UNWATCHED)
  {
    bReturn = true;

    g_PVRRecordings->SetRecordingsPlayCount(item, 0);

    UpdateData();
  }

  return bReturn;
}

void CGUIWindowPVRRecordings::BeforeUpdate(const CStdString &strDirectory)
{
  if (m_thumbLoader.IsLoading())
    m_thumbLoader.StopThread();
}

void CGUIWindowPVRRecordings::AfterUpdate(CFileItemList& items)
{
  if (!items.IsEmpty())
  {
    CFileItemList files;
    for (int i = 0; i < items.Size(); i++)
    {
      CFileItemPtr pItem = items[i];
      if (!pItem->m_bIsFolder)
        files.Add(pItem);
    }

    if (!files.IsEmpty())
    {
      files.SetPath(items.GetPath());
      if(m_database.Open())
      {
        if (g_PVRRecordings->HasAllRecordingsPathExtension(files.GetPath()))
        {
          // Build a map of all files belonging to common subdirectories and call
          // LoadVideoInfo for each item list
          typedef boost::shared_ptr<CFileItemList> CFileItemListPtr;
          typedef std::map<CStdString, CFileItemListPtr> DirectoryMap;

          DirectoryMap directory_map;
          for (int i = 0; i < files.Size(); i++)
          {
            CStdString strDirectory = URIUtils::GetDirectory(files[i]->GetPath());
            DirectoryMap::iterator it = directory_map.find(strDirectory);
            if (it == directory_map.end())
              it = directory_map.insert(std::make_pair(
                  strDirectory, CFileItemListPtr(new CFileItemList(strDirectory)))).first;
            it->second->Add(files[i]);
          }

          for (DirectoryMap::iterator it = directory_map.begin(); it != directory_map.end(); it++)
            CGUIWindowVideoNav::LoadVideoInfo(*it->second, m_database, false);
        }
        else
          CGUIWindowVideoNav::LoadVideoInfo(files, m_database, false);
        m_database.Close();
      }
      m_thumbLoader.Load(files);
    }
  }
}
