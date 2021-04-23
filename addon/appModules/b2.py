from ctypes import sizeof, byref, create_string_buffer
import locale
import re

import api
import appModuleHandler
from colors import RGB
import controlTypes
from displayModel import DisplayModelTextInfo, EditableTextDisplayModelTextInfo
import eventHandler
import locationHelper
import mouseHandler
import oleacc
from NVDAObjects.behaviors import EditableTextWithoutAutoSelectDetection
from NVDAObjects.IAccessible.sysListView32 import ListItem, CBEMAXSTRLEN, LVIF_TEXT, LVIF_COLUMNS
from NVDAObjects.IAccessible import MenuItem
from NVDAObjects.IAccessible.sysTreeView32 import TreeViewItem
from NVDAObjects.window import Window, DisplayModelEditableText
from scriptHandler import script
import textInfos
import ui
import watchdog
import winUser
import winKernel
LVM_GETITEMTEXTA = 4141


def getUnreadTotalCount():
	""" Returns unread vs total count of a messages in a current folder. """
	if api.getStatusBar() is None:
		return "NoStatusBar"
	unreadInfo = ''
	for child in api.getStatusBar().children:
		if child.name is not None and child.name.startswith('Unread:'):
			unreadInfo = child.name
			break
	if unreadInfo == '':
		return "noUnreadCount"
	return int(unreadInfo.split()[1]), int(unreadInfo.split()[-1])


class DanaTextInfo(EditableTextDisplayModelTextInfo):
	""" Needed to make selection announced.
	This has a lot of copied code,
	because at the moment it is not possible to specify only one color as highlight.
	When https://github.com/nvaccess/nvda/pull/10040 would be merged,
	this is going to be reduced to just one line.
	"""

	minHorizontalWhitespace = 10

	def _getSelectionOffsets(self):
		from logHandler import log
		try:
			STRING_TYPE = __builtins__["basestring"]
			log.info(STRING_TYPE)
		except KeyError as k:
			STRING_TYPE = str
		fields = self._storyFieldsAndRects[0]
		startOffset = None
		endOffset = None
		curOffset = 0
		inHighlightChunk = False
		for item in fields:
			if (
				isinstance(item, textInfos.FieldCommand)
				and item.command == "formatChange"
				and item.field.get('color', None) == RGB(red=255, green=255, blue=255)
			):
				inHighlightChunk = True
				if startOffset is None:
					startOffset = curOffset
			elif isinstance(item, STRING_TYPE):
				try:
					import textUtils
					curOffset += textUtils.WideStringOffsetConverter(item).wideStringLength
				except ImportError:
					curOffset += len(item)
				if inHighlightChunk:
					endOffset = curOffset
			else:
				inHighlightChunk = False
		if startOffset is not None and endOffset is not None:
			return (startOffset, endOffset)
		offset = self._getCaretOffset()
		return offset, offset


class DanaEdit(EditableTextWithoutAutoSelectDetection, Window):
	"""We cannot inherit from  DisplayModelEditableText because we are not informed about selection changes.
	"""

	TextInfo = DanaTextInfo
	role = controlTypes.ROLE_EDITABLETEXT

	def event_valueChange(self):
		# Don't report value changes for editable text fields.
		pass

	def _get_isReadOnly(self):
		if self.windowControlID == 103 and self.parent and not self.parent.previous:
			return False
		return True


class FolderTreeViewItem(TreeViewItem):
	""" For the treewiev containing folders.
Annoingly Becky adds expanded state to each not collapsed item.
Remove it from those which do not have children.
	"""

	def _get_states(self):
		states = super(FolderTreeViewItem, self)._get_states()
		if super(FolderTreeViewItem, self)._get_childCount() == 0 and controlTypes.STATE_EXPANDED in states:
			states.remove(controlTypes.STATE_EXPANDED)
		return states

	def _get_shouldAllowIAccessibleFocusEvent(self):
		if controlTypes.STATE_SELECTABLE in self.states and controlTypes.STATE_SELECTED not in self.states:
			return False
		return super(FolderTreeViewItem, self)._get_shouldAllowIAccessibleFocusEvent()

	def _get_unreadInfo(self):
		obj = self
		while obj and obj.role != controlTypes.ROLE_TREEVIEW:
			obj = obj.parent
		rootTreeViewLocation = obj.location
		rect = getattr(textInfos, "Rect", locationHelper.RectLTRB)(
			self.location.left,
			self.location.top,
			self.location.left + rootTreeViewLocation.width,
			self.location.top + self.location.height
		)
		screenContent = DisplayModelTextInfo(obj, rect).text
		unreadInfoFinder = u"^{}\\((\\d+)\\)$".format(self.name)
		if re.match(unreadInfoFinder, screenContent):
			return re.match(unreadInfoFinder, screenContent).group(1)
		else:
			if screenContent != self.name:
				from logHandler import log
				log.info("Failed to get proper displaytext. Got: {}".format(screenContent))
			return None

	def _get_description(self):
		unreadInfo = self.unreadInfo
		if unreadInfo:
			return "{} unread".format(unreadInfo)
		return None


class messagesContextMenu(MenuItem):

	def _get_name(self):
		""" Shortcut of each menu item is replicated in the name.
		Even though it would be easier to just get rid of the shortcut, this approach is more correct.
		"""
		name = super(messagesContextMenu, self)._get_name()
		hotKey = super(messagesContextMenu, self)._get_keyboardShortcut()
		return name.lstrip("{}: ".format(hotKey.upper()))

	def _get_states(self):
		""" If message is received in current account it has always a checked state.
		Discard it here and take care of it when creating a description.
		"""
		states = super(messagesContextMenu, self)._get_states()
		if controlTypes.STATE_CHECKED in states:
			states.remove(controlTypes.STATE_CHECKED)
		return states

	def _get_description(self):
		states = MenuItem._get_states(self)
		if controlTypes.STATE_CHECKED in states or self.IAccessibleChildID == 1:
			return None
		else:
			return "From other account"

	def _get_positionInfo(self):
		currentPos = self.IAccessibleChildID
		total = self.parent.childCount
		if currentPos > 1:
			currentPos -= 1
		return dict(indexInGroup=currentPos, similarItemsInGroup=total - 1)

	@script(
		gestures=["kb:applications", "kb:Shift+F10"]
	)
	def script_deleteItem(self, gesture):
		if self.IAccessibleChildID == 1:
			return
		(left, top, width, height) = self.location
		oldMouseCoords = winUser.getCursorPos()
		x = left + (width // 2)
		y = top + (height // 2)
		winUser.setCursorPos(x, y)
		mouseHandler.executeMouseEvent(winUser.MOUSEEVENTF_RIGHTDOWN, 0, 0)
		mouseHandler.executeMouseEvent(winUser.MOUSEEVENTF_RIGHTUP, 0, 0)
		winUser.setCursorPos(*oldMouseCoords)


class Message(ListItem):

	POSSIBLE_ENCODINGS = ("utf8", locale.getpreferredencoding(), "shift_jis")

	def _getColumnContentRaw(self, index):
		buffer = None
		processHandle = self.processHandle
		internalItem = winKernel.virtualAllocEx(
			processHandle,
			None,
			sizeof(self.LVITEM),
			winKernel.MEM_COMMIT,
			winKernel.PAGE_READWRITE
		)
		try:
			internalText = winKernel.virtualAllocEx(
				processHandle,
				None,
				CBEMAXSTRLEN * 2,
				winKernel.MEM_COMMIT,
				winKernel.PAGE_READWRITE
			)
			try:
				item = self.LVITEM(
					iItem=self.IAccessibleChildID - 1,
					mask=LVIF_TEXT | LVIF_COLUMNS,
					iSubItem=index,
					pszText=internalText,
					cchTextMax=CBEMAXSTRLEN
				)
				winKernel.writeProcessMemory(processHandle, internalItem, byref(item), sizeof(self.LVITEM), None)
				len = watchdog.cancellableSendMessage(
					self.windowHandle,
					LVM_GETITEMTEXTA,
					(self.IAccessibleChildID - 1),
					internalItem
				)
				if len:
					winKernel.readProcessMemory(processHandle, internalItem, byref(item), sizeof(self.LVITEM), None)
					buffer = create_string_buffer(len)
					winKernel.readProcessMemory(processHandle, item.pszText, buffer, sizeof(buffer), None)
			finally:
				winKernel.virtualFreeEx(processHandle, internalText, 0, winKernel.MEM_RELEASE)
		finally:
			winKernel.virtualFreeEx(processHandle, internalItem, 0, winKernel.MEM_RELEASE)
		if buffer:
			for encoding in self.POSSIBLE_ENCODINGS:
				try:
					return buffer.value.decode(encoding)
				except UnicodeDecodeError:
					continue
			return buffer.decode("unicode_escape")
		else:
			return None


class AppModule(appModuleHandler.AppModule):

	def chooseNVDAObjectOverlayClasses(self, obj, clsList):
		if obj.windowClassName == 'DanaEditWindowClass' and obj.IAccessibleRole == oleacc.ROLE_SYSTEM_CLIENT:
			try:
				clsList.remove(DisplayModelEditableText)
			except ValueError:
				pass
			clsList.insert(0, DanaEdit)
			return
		if(
			obj.windowClassName == 'SysTreeView32'
			and obj.role == controlTypes.ROLE_TREEVIEWITEM
			and obj.windowControlID == 1000
		):
			clsList.insert(0, FolderTreeViewItem)
			return
		if obj.windowClassName == '#32768' and obj.role == controlTypes.ROLE_MENUITEM:
			if (
				obj.parent
				and obj.parent.displayText
				and obj.parent.displayText.startswith('** Clear All (To delete one by one, right click the item.) **')
			):
				clsList.insert(0, messagesContextMenu)
				return
		if (
			obj.windowClassName == 'SysListView32'
			and obj.role == controlTypes.ROLE_LISTITEM
			and obj.windowControlID in (59648, 59649, 59664)
		):
			clsList.insert(0, Message)
			return

	def isInMainWindow(self):
		return (
			api.getForegroundObject() is not None
			and api.getForegroundObject().name is not None
			and api.getForegroundObject().name.endswith(' - Becky!')
		)

	@script(
		gesture="kb:NVDA+shift+U",
	)
	def script_unreadTotalInfo(self, gesture):
		if not self.isInMainWindow():
			return
		result = getUnreadTotalCount()
		msg = ''
		if result == "NoStatusBar":
			msg = "Status bar is not visible. Perhaps it is  disabled in the view menu?"
		elif result == "noUnreadCount":
			msg = "Cannot locate unread info in the status bar"
		elif all(item == 0 for item in result):
			msg = "Empty folder"
		elif result[0] == 0:
			msg = "%d total" % result[1]
		else:
			msg = "%d unread, %d total." % (result[0], result[1])
		ui.message(msg)
