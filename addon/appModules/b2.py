from ctypes import sizeof, byref, create_string_buffer
import locale
import operator
import re

import six

import api
import appModuleHandler
from colors import RGB
import controlTypes
from displayModel import DisplayModelTextInfo, EditableTextDisplayModelTextInfo
import locationHelper
from logHandler import log
import mouseHandler
import oleacc
from NVDAObjects.behaviors import EditableTextWithoutAutoSelectDetection
import NVDAObjects.IAccessible
from NVDAObjects.IAccessible.sysListView32 import ListItem, CBEMAXSTRLEN, LVIF_TEXT, LVIF_COLUMNS
from NVDAObjects.IAccessible import IAccessible, MenuItem
from NVDAObjects.IAccessible.sysTreeView32 import TreeViewItem
from NVDAObjects.window import Window, DisplayModelEditableText
from scriptHandler import script
import textInfos
import ui
import watchdog
import windowUtils
import winUser
import winKernel


# Constants:

LVM_GETITEMTEXTA = 4141

BECKY_SCRIPT_CATEGORY = "Becky"


class EnhancedGetter(object):

	def __init__(self, modWithAttrs, attrCommonPrefix, alternativeNameFactories):
		super(EnhancedGetter, self).__init__()
		self.mod = modWithAttrs
		self.attrCommonPrefix = attrCommonPrefix
		self.alternativeNameFactories = alternativeNameFactories

	def __getattr__(self, attrName):
		for aliasNameMaker in self.alternativeNameFactories:
			try:
				return operator.attrgetter(aliasNameMaker(self.attrCommonPrefix, attrName))(self.mod)
			except AttributeError:
				continue
		raise AttributeError("Attribute {} not found!".format(attrName))


class ControlTypesCompatWrapper(object):

	_ALIAS_FACTORIES = (
		lambda attrPrefix, attrName: ".".join((attrPrefix.capitalize(), attrName.upper())),
		lambda attrPrefix, attrName: "_".join((attrPrefix.upper(), attrName.upper()))
	)

	def __init__(self):
		super(ControlTypesCompatWrapper, self).__init__()
		self.Role = EnhancedGetter(
			controlTypes,
			"role",
			self._ALIAS_FACTORIES
		)
		self.State = EnhancedGetter(
			controlTypes,
			"state",
			self._ALIAS_FACTORIES
		)


CTWRAPPER = ControlTypesCompatWrapper()


class NoUnreadInfo(Exception):

	pass


class StatusBarNotVisible(Exception):

	pass


def getUnreadTotalCount():
	""" Returns unread vs total count of a messages in a current folder. """
	try:
		statusBarHandle = windowUtils.findDescendantWindow(
			api.getForegroundObject().windowHandle, visible=True, className="msctls_statusbar32"
		)
	except LookupError:
		raise StatusBarNotVisible
	statusBarObj = NVDAObjects.IAccessible.getNVDAObjectFromEvent(statusBarHandle, winUser.OBJID_CLIENT, 0)
	unreadInfo = ''
	for child in statusBarObj.children:
		if child.name is not None and child.name.startswith('Unread:'):
			unreadInfo = child.name
			break
	if unreadInfo == '':
		raise NoUnreadInfo
	return int(unreadInfo.split()[1]), int(unreadInfo.split()[-1])


class BeckyMainFrame(IAccessible):

	@script(
		gesture="kb:NVDA+shift+U",
		canPropagate=True,
		category=BECKY_SCRIPT_CATEGORY,
		description="Reports amount of unread and all messages in the current folder"
	)
	def script_unreadTotalInfo(self, gesture):
		try:
			result = getUnreadTotalCount()
			if all(item == 0 for item in result):
				msg = "Empty folder"
			elif result[0] == 0:
				msg = "%d total" % result[1]
			else:
				msg = "%d unread, %d total." % (result[0], result[1])
		except StatusBarNotVisible:
			msg = "Status bar is not visible. Perhaps it is  disabled in the view menu?"
		except NoUnreadInfo:
			msg = "Cannot locate unread info in the status bar"
		ui.message(msg)


class BeckyComposeFrame(IAccessible):

	@script(
		gesture="kb:NVDA+shift+a",
		canPropagate=True,
		category=BECKY_SCRIPT_CATEGORY,
		description="Moves focus to the list of attachments if it is visible"
	)
	def script_focusAttachmentsList(self, gesture):
		attachmentsListFound = False
		try:
			attachmentsListHandle = windowUtils.findDescendantWindow(
				api.getForegroundObject().windowHandle, visible=True, className="SysListView32", controlID=1002
			)
			attachmentsListObj = NVDAObjects.IAccessible.getNVDAObjectFromEvent(
				attachmentsListHandle, winUser.OBJID_CLIENT, 0
			)
			if CTWRAPPER.State.INVISIBLE in attachmentsListObj.parent.states:
				attachmentsListFound = False
			else:
				attachmentsListFound = True
				attachmentsListObj.firstChild.setFocus()
		except LookupError:
			attachmentsListFound = False
		if not attachmentsListFound:
			ui.message("Current message has no attachments")


class DanaTextInfo(EditableTextDisplayModelTextInfo):
	""" Needed to make selection announced.
	This has a lot of copied code,
	because at the moment it is not possible to specify only one color as highlight.
	When https://github.com/nvaccess/nvda/pull/10040 would be merged,
	this is going to be reduced to just one line.
	"""

	minHorizontalWhitespace = 10

	def _getSelectionOffsets(self):
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
			elif isinstance(item, six.string_types):
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
	role = CTWRAPPER.Role.EDITABLETEXT

	def event_valueChange(self):
		# Don't report value changes for editable text fields.
		pass

	def _get_isReadOnly(self):
		return self.windowControlID != 103 or api.getForegroundObject().windowClassName == "Becky2MainFrame"

	@staticmethod
	def _activateURLAtPos(pos):
		oldMouseCoords = winUser.getCursorPos()
		winUser.setCursorPos(pos.pointAtStart.x, pos.pointAtStart.y)
		mouseHandler.executeMouseEvent(winUser.MOUSEEVENTF_LEFTDOWN, 0, 0)
		mouseHandler.executeMouseEvent(winUser.MOUSEEVENTF_LEFTUP, 0, 0)
		mouseHandler.executeMouseEvent(winUser.MOUSEEVENTF_LEFTDOWN, 0, 0)
		mouseHandler.executeMouseEvent(winUser.MOUSEEVENTF_LEFTUP, 0, 0)
		winUser.setCursorPos(*oldMouseCoords)

	@staticmethod
	def _isALink(pos):
		for field in pos.getTextWithFields():
			if (
				isinstance(field, textInfos.FieldCommand)
				and field.command == "formatChange"
				and field.field.get('color', None) == RGB(red=0, green=0, blue=192)
			):
				return True
		return False

	@script(
		gesture="kb:enter"
	)
	def script_urlActivate(self, gesture):
		if self.isReadOnly:
			carretPos = self.makeTextInfo(textInfos.POSITION_CARET)
			carretPos.expand(textInfos.UNIT_CHARACTER)
			if self._isALink(carretPos):
				return self._activateURLAtPos(carretPos)
			ui.message("Not on a link")
		else:
			gesture.send()
			return


class FolderTreeViewItem(TreeViewItem):
	""" For the treewiev containing folders.
Annoingly Becky adds expanded state to each not collapsed item.
Remove it from those which do not have children.
	"""

	def _get_states(self):
		states = super(FolderTreeViewItem, self)._get_states()
		if super(FolderTreeViewItem, self)._get_childCount() == 0 and CTWRAPPER.State.EXPANDED in states:
			states.remove(CTWRAPPER.State.EXPANDED)
		return states

	def _get_shouldAllowIAccessibleFocusEvent(self):
		if CTWRAPPER.State.SELECTABLE in self.states and CTWRAPPER.State.SELECTED not in self.states:
			return False
		return super(FolderTreeViewItem, self)._get_shouldAllowIAccessibleFocusEvent()

	def _get_unreadInfo(self):
		obj = self
		while obj and obj.role != CTWRAPPER.Role.TREEVIEW:
			obj = obj.parent
		rootTreeViewLocation = obj.location
		rect = getattr(textInfos, "Rect", locationHelper.RectLTRB)(
			self.location.left,
			self.location.top,
			self.location.left + rootTreeViewLocation.width,
			self.location.top + self.location.height
		)
		screenContent = DisplayModelTextInfo(obj, rect).text
		unreadInfoFinder = u"^{}\\((\\d+|\\+)\\)$".format(re.escape(self.name))
		if re.match(unreadInfoFinder, screenContent):
			return re.match(unreadInfoFinder, screenContent).group(1)
		else:
			if screenContent != self.name:
				log.error("Failed to get proper displaytext. Got: {}".format(screenContent))
			return None

	def _get_description(self):
		unreadInfo = self.unreadInfo
		if unreadInfo:
			if unreadInfo == "+":
				return "Contains unread messages"
			else:
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
		if CTWRAPPER.State.CHECKED in states:
			states.remove(CTWRAPPER.State.CHECKED)
		return states

	def _get_description(self):
		states = MenuItem._get_states(self)
		if CTWRAPPER.State.CHECKED in states or self.IAccessibleChildID == 1:
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

	POSSIBLE_ENCODINGS = ("utf8", locale.getpreferredencoding(), "1251", "shift_jis", "gb18030")

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
			colContentBytes = buffer.value
			left, top, width, height = self._getColumnLocationRaw(index)
			displayedColContent = DisplayModelTextInfo(
				self,
				getattr(textInfos, "Rect", locationHelper.RectLTRB)(left, top, left + width, top + height)
			).text
			COL_INCOMPLETE_END = "..."
			if u'\uffff' in displayedColContent:
				displayedColContent = []
			else:
				displayedColContent = displayedColContent.split(" ")
				if(
					displayedColContent[-1] == COL_INCOMPLETE_END
					or displayedColContent[-1].endswith(COL_INCOMPLETE_END)
					or not displayedColContent[-1]
				):
					displayedColContent = displayedColContent[:-1]
			for encoding in self.POSSIBLE_ENCODINGS:
				try:
					decodedColContent = colContentBytes.decode(encoding)
					if self._displayedContentMatchesRetrieved(displayedColContent, decodedColContent):
						return decodedColContent
					continue
				except UnicodeDecodeError:
					continue
			try:
				return colContentBytes.decode("utf8")
			except UnicodeDecodeError:
				return colContentBytes.decode("unicode_escape")
		else:
			return None

	@staticmethod
	def _displayedContentMatchesRetrieved(screenContent, programaticContent):
		if screenContent:
			programaticContent = programaticContent.split(" ")[:len(screenContent)]
			return screenContent == programaticContent
		return True


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
			and obj.role == CTWRAPPER.Role.TREEVIEWITEM
			and obj.windowControlID == 1000
		):
			clsList.insert(0, FolderTreeViewItem)
			return
		if obj.windowClassName == '#32768' and obj.role == CTWRAPPER.Role.MENUITEM:
			if (
				obj.parent
				and obj.parent.displayText
				and obj.parent.displayText.startswith('** Clear All (To delete one by one, right click the item.) **')
			):
				clsList.insert(0, messagesContextMenu)
				return
		if (
			obj.windowClassName == 'SysListView32'
			and obj.role == CTWRAPPER.Role.LISTITEM
			and obj.windowControlID in (59648, 59649, 59664)
		):
			clsList.insert(0, Message)
			return
		if obj.windowClassName == 'Becky2MainFrame' and obj.IAccessibleRole == oleacc.ROLE_SYSTEM_CLIENT:
			clsList.insert(0, BeckyMainFrame)
			return
		if obj.windowClassName == 'Becky2ComposeFrame' and obj.IAccessibleRole == oleacc.ROLE_SYSTEM_CLIENT:
			clsList.insert(0, BeckyComposeFrame)
			return
