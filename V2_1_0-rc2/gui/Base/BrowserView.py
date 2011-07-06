# -*- coding: utf-8 -*-
#
#-------------------------------------------------------------------------------
#
#     This file is part of the Code_Saturne User Interface, element of the
#     Code_Saturne CFD tool.
#
#     Copyright (C) 1998-2009 EDF S.A., France
#
#     contact: saturne-support@edf.fr
#
#     The Code_Saturne User Interface is free software; you can redistribute it
#     and/or modify it under the terms of the GNU General Public License
#     as published by the Free Software Foundation; either version 2 of
#     the License, or (at your option) any later version.
#
#     The Code_Saturne User Interface is distributed in the hope that it will be
#     useful, but WITHOUT ANY WARRANTY; without even the implied warranty
#     of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#     GNU General Public License for more details.
#
#     You should have received a copy of the GNU General Public License
#     along with the Code_Saturne Kernel; if not, write to the
#     Free Software Foundation, Inc.,
#     51 Franklin St, Fifth Floor,
#     Boston, MA  02110-1301  USA
#
#-------------------------------------------------------------------------------

"""
This module defines the following classes:
- BrowserView
"""

#-------------------------------------------------------------------------------
# Standard modules
#-------------------------------------------------------------------------------

import sys, logging

#-------------------------------------------------------------------------------
# Third-party modules
#-------------------------------------------------------------------------------

from PyQt4.QtCore import *
from PyQt4.QtGui  import *

#-------------------------------------------------------------------------------
# Application modules import
#-------------------------------------------------------------------------------

from Base.BrowserForm import Ui_BrowserForm
from Base.Toolbox import GuiParam, displaySelectedPage

#-------------------------------------------------------------------------------
# log config
#-------------------------------------------------------------------------------

logging.basicConfig()
log = logging.getLogger("BrowserView")
log.setLevel(GuiParam.DEBUG)

#-------------------------------------------------------------------------------
#
#-------------------------------------------------------------------------------

class TreeItem:
    def __init__(self, data, typename, parent=None):
        self.parentItem = parent
        self.itemData = data
        self.itemType = typename
        self.itemIcon = None
        self.childItems = []

    def appendChild(self, item):
        self.childItems.append(item)

    def child(self, row):
        return self.childItems[row]

    def childCount(self):
        return len(self.childItems)

    def columnCount(self):
        return len(self.itemData)

    def data(self, column):
        return self.itemData[column]

    def parent(self):
        return self.parentItem

    def row(self):
        if self.parentItem:
            return self.parentItem.childItems.index(self)

        return 0

#-------------------------------------------------------------------------------
#
#-------------------------------------------------------------------------------

class TreeModel(QAbstractItemModel):
    """A model representing the widget tree structure.
    """
    def __init__(self, data, parent=None):
        """Constructs a new item model with the given I{parent}.

        @type data: C{QString}
        @param data: content of the new item
        @type parent: C{QObject} or C{None}
        @param parent: parent of the new item
        """
        QAbstractItemModel.__init__(self, parent)

        rootData = []
        rootData.append(QVariant("Pages"))
        self.rootItem = TreeItem(rootData, "folder")
        self.populateModel(data.split("\n"), self.rootItem)


    def columnCount(self, parent):
        """Returns the number of columns for the children of the given I{parent}.

        @type parent: C{QModelIndex}
        @param parent: parent of the item
        @return: C{int}
        """
        if parent.isValid():
            return parent.internalPointer().columnCount()
        else:
            return self.rootItem.columnCount()


    def data(self, index, role):
        """Returns the data stored under the given I{role} for the item referred to by the I{index}.

        @type index: C{QModelIndex}
        @param index: used to locate data in a data model
        @type role: C{Qt.ItemDataRole}
        @param role: used by the view to indicate to the model which type of data it needs
        @return: C{QVariant}
        """
        if not index.isValid():
            return QVariant()

        item = index.internalPointer()
        column = index.column()

        if role == Qt.DisplayRole:
            # return text for columns
            if column == 0:
                return QVariant(item.itemData[column])

        elif role == Qt.DecorationRole:
            # return icon for first column
            if column == 0:
                style = QWidget().style()
                if item.itemType == "folder-new":
                    icon = style.standardIcon(QStyle.SP_FileDialogNewFolder)
                elif item.itemType == "folder-close":
                    icon = style.standardIcon(QStyle.SP_DirClosedIcon)
                elif item.itemType == "folder-open":
                    icon = style.standardIcon(QStyle.SP_DirOpenIcon)
                elif item.itemType == "file-open":
                    icon = style.standardIcon(QStyle.SP_FileIcon)
                elif item.itemType == "file-new":
                    icon = style.standardIcon(QStyle.SP_FileLinkIcon)
                return QVariant(icon)

        # return nothing
        return QVariant()


    def flags(self, index):
        """What we can do with the item.

        @type index: C{QModelIndex}
        @param index: used to locate data in a data model
        """
        if not index.isValid():
            return Qt.ItemIsEnabled

        flags = Qt.ItemIsEnabled | Qt.ItemIsSelectable

#       if index.internalPointer() is not self.rootItem and index.column() == 0:
#            # allow items other than root to be edited
#            flags |= Qt.ItemIsEditable
        return flags


    def headerData(self, section, orientation, role):
        """Return the header of the tree.*

        @return: C{QVariant}
        """
        if orientation == Qt.Horizontal and role == Qt.DisplayRole:
            return self.rootItem.data(section)

        return QVariant()


    def index(self, row, column, parent):
        """Returns the index of the item in the model specified by the given I{row}, I{column} and I{parent} index.

        @type row: C{int}
        @param row: row of the item
        @type column: C{int}
        @param column: column of the item
        @type parent: C{QModelIndex}
        @param parent: parent of the item
        @return: C{QModelIndex}
        """
        if not parent.isValid():
            parentItem = self.rootItem
        else:
            parentItem = parent.internalPointer()

        #FIXME: why childItem can be None?
        try:
            childItem = parentItem.child(row)
        except:
            childItem = None

        if childItem:
            return self.createIndex(row, column, childItem)
        else:
            return QModelIndex()


    def parent(self, index):
        """Returns the parent of the model item with the given index.

        @type index: C{QModelIndex}
        @param index: index of the child
        @return: C{QModelIndex}
        """
        if not index.isValid():
            return QModelIndex()

        childItem = index.internalPointer()
        parentItem = childItem.parent()

        if parentItem == self.rootItem:
            return QModelIndex()

        return self.createIndex(parentItem.row(), 0, parentItem)


    def rowCount(self, parent):
        """Returns the number of rows under the given I{parent}.

        @type parent: C{QModelIndex}
        @param parent: parent of the item
        @return: C{int}
        """

        if not parent.isValid():
            parentItem = self.rootItem
        else:
            parentItem = parent.internalPointer()

        return parentItem.childCount()


    def match(self, start, role, value, hits, flags):
        """
        @type start: C{QModelIndex}
        @type role: C{Qt.ItemDataRole}
        @type value: C{QVarient}
        @type hits: C{int}
        @type flags: C{Qt.MatchFlag}
        """
        result = []
        p = self.parent(start)
        st = start.row()
        to = self.rowCount(p)

        for r in range(st, to):

            index = self.index(r, start.column(), p)
            if not index.isValid():
                 pass
            v = self.data(index, role)

            if flags == Qt.MatchExactly:
                if value == v:
                    result.append(index)
            else:
                raise ValueError("This flags is not implemented")

            if self.hasChildren(index):
                result += self.match(self.index(0, index.column(), index), role, value, hits, flags)

        return result


    def itemLocalization(self, data, role=Qt.DisplayRole):
        """
        """
        info = []
        search_item = QVariant(QString(data))
        start = self.index(0, 0, QModelIndex())
        indexList = self.match(start, role, search_item, -1, Qt.MatchExactly)

        for index in indexList:
            item   = index.internalPointer()
            column = index.column()
            row    = index.row()
            parent = self.parent(index)

            info.append( (row, column, parent) )

        return info


    def populateModel(self, lines, parent):
        """
        @type lines: C{QString}
        @param lines:
        @type parent: C{QModelIndex}
        @param parent: parent of the item
        """
        parents = []
        indentations = []

        parents.append(parent)
        indentations.append(0)

        for number in range(len(lines)):
            position = 0
            while position < len(lines[number]):
                if lines[number][position] != " ":
                    break
                position += 1

            lineData = lines[number][position:].trimmed()

            if not lineData.isEmpty():
                # Read the column data from the rest of the line.
                columnStrings = lineData.split("\t", QString.SkipEmptyParts)
                columnData = []
                for column in range(0, len(columnStrings)):
                    columnData.append(columnStrings[column])

                if position == 0:
                    typename = "folder-new"
                else:
                    typename = "file-new"

                if position > indentations[-1]:
                    # The last child of the current parent is now the new parent
                    # unless the current parent has no children.
                    if parents[-1].childCount() > 0:
                        parents.append(parents[-1].child(parents[-1].childCount() - 1))
                        indentations.append(position)

                else:
                    while position < indentations[-1] and len(parents) > 0:
                        parents.pop()
                        indentations.pop()

                # Append a new item to the current parent's list of children.
                parents[-1].appendChild(TreeItem(columnData, typename, parents[-1]))

#-------------------------------------------------------------------------------
#
#-------------------------------------------------------------------------------

class BrowserView(QWidget, Ui_BrowserForm):
    """
    Class for the browser widget
    """
    def __init__(self):
        """
        Constructor
        """
        QWidget.__init__(self)

        Ui_BrowserForm.__init__(self)
        self.setupUi(self)

        tree = self._browser()
        self.model = TreeModel(QString(tree))

        self.treeView.setModel(self.model)
        self.treeView.header().hide()
        self.treeView.setAnimated(True)
        #self.treeView.setMinimumSize(QSize(200, 300))
        self.treeView.setVerticalScrollBarPolicy(Qt.ScrollBarAsNeeded)
        self.treeView.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOn)
        self.treeView.setAlternatingRowColors(True)
        self.treeView.setWindowTitle("Simple Tree Model")

        # Popup menu
        self.treeView.setContextMenuPolicy(Qt.CustomContextMenu)
        self.connect(self.treeView, SIGNAL("customContextMenuRequested(QPoint)"), self.displayPopup)

        # Receive change in selection
        self.connect(self.treeView, SIGNAL('pressed(const QModelIndex &)'), self.onItemPressed)
        self.connect(self.treeView, SIGNAL('expanded(const QModelIndex &)'), self.onFolderOpen)
        self.connect(self.treeView, SIGNAL('collapsed(const QModelIndex &)'), self.onFolderClose)


    def _browser(self):
        tree ="""
    Identity and paths
Calculation environment
    Meshes selection
    Mesh quality criteria
Storage system description
    Storage system type
    Storage system geometry
    Inlet description
    Outlet description
    Network description
Thermohydraulic parameters
    Hydraulic load
    Thermal load
Thermophysical models
    Calculation features
    Mobile mesh
    Turbulence models
    Thermal model
    Gas combustion
    Current species
    Pulverized coal combustion
    Electrical models
    Radiative transfers
    Conjugate heat transfer
    Atmospheric flows
Additional scalars
    Definition and initialization
    Physical properties
Physical properties
    Reference values
    Fluid properties
    Gravity, hydrostatic pressure
Volume conditions
    Volume regions definition
    Initialization
    Head losses
    Coriolis Source Terms
Particles and droplets tracking
    Global settings
    Statistics
    Output
Boundary conditions
    Definition of boundary regions
    Boundary conditions
    Particles boundary conditions
    Fluid structure interaction
Numerical parameters
    Time step
    Steady flow management
    Equation parameters
    Global parameters
Calculation control
    Time averages
    Output control
    Volume solution control
    Surface solution control
    Profiles
Calculation management
    Start/Restart
    Prepare batch calculation
"""
        return tree


    def setRowClose(self, string):
        log.debug("setRowClose(): %s" % string)
        itemInfoList = self.model.itemLocalization(string)
        for itemInfo in itemInfoList:
            row    = itemInfo[0]
            column = itemInfo[1]
            parent = itemInfo[2]
            self.treeView.setRowHidden(row, parent, True)


    def setRowOpen(self, string):
        log.debug("setRowOpen(): %s" % string)
        itemInfoList = self.model.itemLocalization(string)
        for itemInfo in itemInfoList:
            row    = itemInfo[0]
            column = itemInfo[1]
            parent = itemInfo[2]
            self.treeView.setRowHidden(row, parent, False)


    def isRowClose(self, string):
        log.debug("isRowClose(): %s" % string)
        itemInfoList = self.model.itemLocalization(string)
        for itemInfo in itemInfoList:
            row    = itemInfo[0]
            column = itemInfo[1]
            parent = itemInfo[2]
            index  = self.model.index(row, column, parent)
            # FIXME: this return should not be in a loop
            return self.treeView.isRowHidden(row, index)


    @pyqtSignature('const QModelIndex &')
    def onItemPressed(self, index):
        item = index.internalPointer()
        if item.itemType == "file-new":
            item.itemType = "file-open"


    @pyqtSignature('const QModelIndex &')
    def onFolderOpen(self, index):
        """
        public slot

        change the item type when the folder is opened

        @type index: C{QModelIndex}
        @param index: index in the model of the selected folder
        """
        item = index.internalPointer()
        if item.itemType == "folder-new" or item.itemType == "folder-close":
            item.itemType = "folder-open"


    @pyqtSignature('const QModelIndex &')
    def onFolderClose(self, index):
        """
        public slot

        change the item type when the folder is closed

        @type index: C{QModelIndex}
        @param index: index in the model of the selected folder
        """
        item = index.internalPointer()
        if item.itemType == "folder-new" or item.itemType == "folder-open":
            item.itemType = "folder-close"


    @pyqtSignature("")
    def displayPopup(self):
        """
        public slot

        create the popup menu of the Browser
        """
        self.fileMenu = QMenu(self.treeView)

        self.actionExpand = QAction(self.tr("Expand"), self.treeView)
        #self.actionExpand.setShortcut(self.tr("F5"))
        self.connect(self.actionExpand, SIGNAL("triggered()"), self.openTreeFolder)

        self.actionCollapse = QAction(self.tr("Collapse"), self.treeView)
        #self.actionCollapse.setShortcut(self.tr("F6"))
        self.connect(self.actionCollapse, SIGNAL("triggered()"), self.closeTreeFolder)

        # ... TODO
        #self.actionWelcome = QAction(self.tr("Welcome page"), self.treeView)

        self.fileMenu.addAction(self.actionExpand)
        self.fileMenu.addAction(self.actionCollapse)

        cursor = QCursor()
        self.fileMenu.popup(cursor.pos())
        self.fileMenu.show()


    def display(self, root, case, stbar, study, tree):
        """
        """
        index = self.treeView.currentIndex()
        item  = index.internalPointer()
        name  = item.itemData[0]
        return displaySelectedPage(name, root, case, stbar, study, tree)


    def isFolder(self):
        """
        Return True if current item is a folder (parent)
        """
        index = self.treeView.currentIndex()
        item  = index.internalPointer()
        return item.childCount() != 0


    def openSingleFolder(self, string):
        """
        Open a single folder of the Tree.
        """
        itemInfoList = self.model.itemLocalization(string)
        for itemInfo in itemInfoList:
            row    = itemInfo[0]
            column = itemInfo[1]
            parent = itemInfo[2]
            index  = self.model.index(row, column, parent)
            self.treeView.expand(index)


    @pyqtSignature("")
    def openTreeFolder(self):
        """
        public slot

        open all folders of the Tree.
        """
        self.treeView.expandAll()

        parent = QModelIndex()
        column = 0
        for row in range(self.model.rowCount(parent)):
            index = self.model.index(row, column, parent)
            self.onFolderOpen(index)

        if hasattr(self, 'case'):
            self.configureTree(self.case)


    def closeSingleFolder(self, string):
        """
        Close a single folder of the Tree.
        """
        itemInfoList = self.model.itemLocalization(string)
        for itemInfo in itemInfoList:
            row    = itemInfo[0]
            column = itemInfo[1]
            parent = itemInfo[2]
            index  = self.model.index(row, column, parent)
            self.treeView.collapse(index)


    @pyqtSignature("")
    def closeTreeFolder(self):
        """
        public slot

        close all folders of the Tree.
        """
        self.treeView.collapseAll()

        parent = QModelIndex()
        column = 0
        for row in range(self.model.rowCount(parent)):
            index = self.model.index(row, column, parent)
            self.onFolderClose(index)


    def configureTree(self, case):
        """
        Public method.
        Configures the browser with users data.
        """
        try:
            import cs_package
        except:
            self.setRowClose(self.tr('Mesh quality criteria'))

        self.setRowClose(self.tr('Particles and droplets tracking'))
        self.setRowClose(self.tr('Gas combustion'))
        self.setRowClose(self.tr('Current species'))
        self.setRowClose(self.tr('Pulverized coal combustion'))
        self.setRowClose(self.tr('Electrical models'))
        self.setRowClose(self.tr('Radiative transfers'))
        self.setRowClose(self.tr('Conjugate heat transfer'))
        self.setRowClose(self.tr('Atmospheric flows'))
        self.setRowClose(self.tr('Radiative boundary conditions'))
        self.setRowClose(self.tr('Particles boundary conditions'))
        self.setRowClose(self.tr('Steady flow management'))
        # self.setRowClose(self.tr('Surface solution control'))
        self.setRowClose(self.tr('Time step'))
        self.setRowClose(self.tr('Storage system description'))
        self.setRowClose(self.tr('Thermohydraulic parameters'))
        self.setRowClose(self.tr('Mobil mesh boundary'))
        self.setRowClose(self.tr('Fluid structure interaction'))

        # Steady flow management

        nodeanal = case.xmlGetNode('analysis_control')
        nodeSteady = nodeanal.xmlGetNode('steady_management')

        if nodeSteady['status'] == 'on':
            self.setRowClose(self.tr('Time step'))
            self.setRowOpen(self.tr('Steady flow management'))
        else:
            nodeSteady['status'] = 'off'
            self.setRowClose(self.tr('Steady flow management'))
            self.setRowOpen(self.tr('Time step'))

        # Multi-phase flow

        nodeLagr = case.xmlGetNode('lagrangian', 'model')

        if nodeLagr and nodeLagr['model'] == "on":
            self.setRowOpen(self.tr('Particles and droplets tracking'))
            self.setRowOpen(self.tr('Particles boundary conditions'))
        else:
            self.setRowClose(self.tr('Particles and droplets tracking'))
            self.setRowClose(self.tr('Particles boundary conditions'))

        # OutputSurfacicView

        node_control = case.xmlGetNode('analysis_control')
        node_out     = node_control.xmlInitNode('output')
        node_bound   = node_out.xmlGetNode('domain_boundary', 'status')

        # FIXME: node_bound = ''
        if node_bound and node_bound['status'] == 'on':
            self.setRowOpen(self.tr('Surface solution control'))

        # Reactive flow

        node0 = case.xmlGetNode('thermophysical_models')
        node1 = node0.xmlGetNode('gas_combustion',     'model')
        node2 = node0.xmlGetNode('pulverized_coal',    'model')
        node3 = node0.xmlGetNode('joule_effect',       'model')
        node4 = node0.xmlGetNode('thermal_scalar',     'model')
        node5 = node0.xmlGetNode('radiative_transfer', 'model')
        node6 = node0.xmlGetNode('atmospheric_flows',  'model')

        if node1['model'] in ('ebu', '3p'):
            self.setRowClose(self.tr('Thermal model'))
            self.setRowOpen(self.tr('Gas combustion'))
            self.setRowOpen(self.tr('Radiative transfers'))
            self.setRowOpen(self.tr('Conjugate heat transfer'))
            if node5.xmlGetAttribute('model') != 'off':
                self.setRowOpen(self.tr('Radiative boundary conditions'))

        elif node2['model'] in ('coal_homo', 'coal_homo2'):
            self.setRowClose(self.tr('Thermal model'))
            self.setRowOpen(self.tr('Current species'))
            self.setRowOpen(self.tr('Pulverized coal combustion'))
            self.setRowOpen(self.tr('Radiative transfers'))
            self.setRowOpen(self.tr('Conjugate heat transfer'))
            if node5.xmlGetAttribute('model') != 'off':
                self.setRowOpen(self.tr('Radiative boundary conditions'))

        elif node3['model'] in ('joule', 'arc'):
            self.setRowClose(self.tr('Thermal model'))
            self.setRowOpen(self.tr('Electrical models'))
            self.setRowOpen(self.tr('Radiative transfers'))
            self.setRowOpen(self.tr('Conjugate heat transfer'))
            if node5.xmlGetAttribute('model') != 'off':
                self.setRowOpen(self.tr('Radiative boundary conditions'))

        elif node6 and node6['model'] != 'off':
            self.setRowClose(self.tr('Thermal model'))
            self.setRowOpen(self.tr('Atmospheric flows'))
            self.setRowOpen(self.tr('Radiative transfers'))
            self.setRowOpen(self.tr('Conjugate heat transfer'))
            if node5.xmlGetAttribute('model') != 'off':
                self.setRowOpen(self.tr('Radiative boundary conditions'))

        else:
            self.setRowOpen(self.tr('Thermal model'))
            if node4.xmlGetAttribute('model') != 'off':
                self.setRowOpen(self.tr('Radiative transfers'))
                self.setRowOpen(self.tr('Conjugate heat transfer'))
                if node5.xmlGetAttribute('model') != 'off':
                    self.setRowOpen(self.tr('Radiative boundary conditions'))

        node7 = node0.xmlGetNode('ale_method', 'status')
        if node7 and node7['status'] == 'on':
            self.setRowOpen(self.tr('Mobil mesh boundary'))
            self.setRowOpen(self.tr('Fluid structure interaction'))

        self.__hideRow()


    def __hideRow(self):
        """Only for developpement purpose"""
        #self.setRowClose(self.tr('Head losses'))
        self.setRowClose(self.tr('Current species'))


    def tr(self, text):
        """
        Translation
        """
        return text

#-------------------------------------------------------------------------------
# Testing part
#-------------------------------------------------------------------------------

if __name__ == "__main__":
    app = QApplication(sys.argv)
    BrowserView = BrowserView()
    BrowserView.show()
    sys.exit(app.exec_())

#-------------------------------------------------------------------------------
# End
#-------------------------------------------------------------------------------
