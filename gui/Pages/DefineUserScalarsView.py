# -*- coding: utf-8 -*-

#-------------------------------------------------------------------------------

# This file is part of Code_Saturne, a general-purpose CFD tool.
#
# Copyright (C) 1998-2012 EDF S.A.
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc., 51 Franklin
# Street, Fifth Floor, Boston, MA 02110-1301, USA.

#-------------------------------------------------------------------------------

"""
This module defines the 'Additional user's scalars' page.

This module contains the following classes:
- LabelDelegate
- VarianceNameDelegate
- VarianceDelegate
- StandardItemModelScalars
- DefineUserScalarsView
"""

#-------------------------------------------------------------------------------
# Standard modules
#-------------------------------------------------------------------------------

import logging

#-------------------------------------------------------------------------------
# Third-party modules
#-------------------------------------------------------------------------------

from PyQt4.QtCore import *
from PyQt4.QtGui  import *

#-------------------------------------------------------------------------------
# Application modules import
#-------------------------------------------------------------------------------

from Pages.DefineUserScalarsForm import Ui_DefineUserScalarsForm

from Pages.LocalizationModel import LocalizationModel
from Pages.DefineUserScalarsModel import DefineUserScalarsModel

from Base.Common import LABEL_LENGTH_MAX
from Base.Toolbox import GuiParam
from Base.QtPage import ComboModel, DoubleValidator, RegExpValidator

#-------------------------------------------------------------------------------
# log config
#-------------------------------------------------------------------------------

logging.basicConfig()
log = logging.getLogger("DefineUserScalarsView")
log.setLevel(GuiParam.DEBUG)

#-------------------------------------------------------------------------------
# Line edit delegate for the label
#-------------------------------------------------------------------------------

class LabelDelegate(QItemDelegate):
    """
    Use of a QLineEdit in the table.
    """
    def __init__(self, parent=None):
        QItemDelegate.__init__(self, parent)
        self.parent = parent
        self.old_plabel = ""


    def createEditor(self, parent, option, index):
        editor = QLineEdit(parent)
        self.old_label = ""
        #editor.installEventFilter(self)
        rx = "[_a-zA-Z][_A-Za-z0-9]{1," + str(LABEL_LENGTH_MAX-1) + "}"
        self.regExp = QRegExp(rx)
        v = RegExpValidator(editor, self.regExp)
        editor.setValidator(v)
        return editor


    def setEditorData(self, editor, index):
        value = index.model().data(index, Qt.DisplayRole).toString()
        self.old_plabel = str(value)
        editor.setText(value)


    def setModelData(self, editor, model, index):
        if not editor.isModified():
            return

        if editor.validator().state == QValidator.Acceptable:
            new_plabel = str(editor.text())

            if new_plabel in model.mdl.getScalarLabelsList():
                default = {}
                default['label']  = self.old_plabel
                default['list']   = model.mdl.getScalarLabelsList()
                default['regexp'] = self.regExp
                log.debug("setModelData -> default = %s" % default)

                from Pages.VerifyExistenceLabelDialogView import VerifyExistenceLabelDialogView
                dialog = VerifyExistenceLabelDialogView(self.parent, default)
                if dialog.exec_():
                    result = dialog.get_result()
                    new_plabel = result['label']
                    log.debug("setModelData -> result = %s" % result)
                else:
                    new_plabel = self.old_plabel

            model.setData(index, QVariant(QString(new_plabel)), Qt.DisplayRole)

#-------------------------------------------------------------------------------
# Line edit delegate for the variance name
#-------------------------------------------------------------------------------

class VarianceNameDelegate(QItemDelegate):
    """
    Use of a QLineEdit in the table.
    """
    def __init__(self, parent=None):
        QItemDelegate.__init__(self, parent)
        self.parent = parent
        self.old_plabel = ""


    def createEditor(self, parent, option, index):
        editor = QLineEdit(parent)
        self.old_label = ""
        rx = "[_a-zA-Z][_A-Za-z0-9]{1," + str(LABEL_LENGTH_MAX-1) + "}"
        self.regExp = QRegExp(rx)
        v = RegExpValidator(editor, self.regExp)
        editor.setValidator(v)
        return editor


    def setEditorData(self, editor, index):
        value = index.model().data(index, Qt.DisplayRole).toString()
        self.old_plabel = str(value)
        editor.setText(value)


    def setModelData(self, editor, model, index):
        if not editor.isModified():
            return

        if editor.validator().state == QValidator.Acceptable:
            new_plabel = str(editor.text())

            if new_plabel in model.mdl.getScalarLabelsList():
                default = {}
                default['label']  = self.old_plabel
                default['list']   = model.mdl.getScalarLabelsList()
                default['regexp'] = self.regExp
                log.debug("setModelData -> default = %s" % default)

                from Pages.VerifyExistenceLabelDialogView import VerifyExistenceLabelDialogView
                dialog = VerifyExistenceLabelDialogView(self.parent, default)
                if dialog.exec_():
                    result = dialog.get_result()
                    new_plabel = result['label']
                    log.debug("setModelData -> result = %s" % result)
                else:
                    new_plabel = self.old_plabel

            model.setData(index, QVariant(QString(new_plabel)), Qt.DisplayRole)


#-------------------------------------------------------------------------------
# Combo box delegate for the variance
#-------------------------------------------------------------------------------

class VarianceDelegate(QItemDelegate):
    """
    Use of a combo box in the table.
    """
    def __init__(self, parent):
        super(VarianceDelegate, self).__init__(parent)
        self.parent   = parent


    def createEditor(self, parent, option, index):
        editor = QComboBox(parent)
        self.modelCombo = ComboModel(editor, 1, 1)
        editor.installEventFilter(self)
        return editor


    def setEditorData(self, editor, index):
        l1 = index.model().mdl.getScalarLabelsList()
        for s in index.model().mdl.getScalarsVarianceList():
            if s in l1: l1.remove(s)

        for s in l1:
            self.modelCombo.addItem(s, s)


    def setModelData(self, comboBox, model, index):
        txt = str(comboBox.currentText())
        value = self.modelCombo.dicoV2M[txt]
        model.setData(index, QVariant(value), Qt.DisplayRole)


    def tr(self, text):
        """
        Translation
        """
        return text

#-------------------------------------------------------------------------------
# StandarItemModel class
#-------------------------------------------------------------------------------

class StandardItemModelScalars(QStandardItemModel):
    """
    """
    def __init__(self, parent, mdl):
        """
        """
        QStandardItemModel.__init__(self)

        self.headers = [self.tr("Name")]

        self.setColumnCount(len(self.headers))

        self.toolTipRole = [self.tr("Code_Saturne keyword: NSCAUS")]

        self._data = []
        self.parent = parent
        self.mdl  = mdl


    def data(self, index, role):
        if not index.isValid():
            return QVariant()

        row = index.row()
        col = index.column()

        if role == Qt.ToolTipRole:
            return QVariant(self.toolTipRole[col])
        if role == Qt.DisplayRole:
            return QVariant(self._data[row][col])

        return QVariant()


    def flags(self, index):
        if not index.isValid():
            return Qt.ItemIsEnabled
        return Qt.ItemIsEnabled | Qt.ItemIsSelectable | Qt.ItemIsEditable


    def headerData(self, section, orientation, role):
        if orientation == Qt.Horizontal and role == Qt.DisplayRole:
            return QVariant(self.headers[section])
        return QVariant()


    def setData(self, index, value, role):
        if not index.isValid():
            return Qt.ItemIsEnabled

        # Update the row in the table
        row = index.row()
        col = index.column()

        # Label
        if col == 0:
            old_plabel = self._data[row][col]
            new_plabel = str(value.toString())
            self._data[row][col] = new_plabel
            self.mdl.renameScalarLabel(old_plabel, new_plabel)

        self.emit(SIGNAL("dataChanged(const QModelIndex &, const QModelIndex &)"), index, index)
        return True


    def getData(self, index):
        row = index.row()
        return self._data[row]


    def newItem(self, existing_label=None):
        """
        Add an item in the table view
        """
        row = self.rowCount()

        label = self.mdl.addUserScalar(existing_label)
        scalar = [label]

        self.setRowCount(row+1)
        self._data.append(scalar)


    def getItem(self, row):
        """
        Return the values for an item.
        """
        [label] = self._data[row]
        return label


    def deleteItem(self, row):
        """
        Delete the row in the model.
        """
        log.debug("deleteItem row = %i " % row)

        del self._data[row]
        row = self.rowCount()
        self.setRowCount(row-1)


#-------------------------------------------------------------------------------
# StandarItemModel class
#-------------------------------------------------------------------------------

class StandardItemModelVariance(QStandardItemModel):
    """
    """
    def __init__(self, parent, mdl):
        """
        """
        QStandardItemModel.__init__(self)

        self.headers = [self.tr("Variance"),
                        self.tr("Species_Name")]

        self.setColumnCount(len(self.headers))

        self.toolTipRole = [self.tr("Code_Saturne keyword: ???"),
                            self.tr("Code_Saturne keyword: ???")]

        self._data = []
        self.parent = parent
        self.mdl  = mdl


    def data(self, index, role):
        if not index.isValid():
            return QVariant()

        row = index.row()
        col = index.column()

        if role == Qt.ToolTipRole:
            return QVariant(self.toolTipRole[col])
        if role == Qt.DisplayRole:
            return QVariant(self._data[row][col])

        return QVariant()


    def flags(self, index):
        if not index.isValid():
            return Qt.ItemIsEnabled
        return Qt.ItemIsEnabled | Qt.ItemIsSelectable | Qt.ItemIsEditable


    def headerData(self, section, orientation, role):
        if orientation == Qt.Horizontal and role == Qt.DisplayRole:
            return QVariant(self.headers[section])
        return QVariant()


    def setData(self, index, value, role):
        if not index.isValid():
            return Qt.ItemIsEnabled

        # Update the row in the table
        row = index.row()
        col = index.column()

        # Label
        if col == 0:
            old_plabel = self._data[row][col]
            new_plabel = str(value.toString())
            self._data[row][col] = new_plabel
            self.mdl.renameScalarLabel(old_plabel, new_plabel)


        # Variance
        elif col == 1:
            variance = str(value.toString())
            self._data[row][col] = variance
            [label, var] = self._data[row]
            self.mdl.setScalarVariance(label,var)

        self.emit(SIGNAL("dataChanged(const QModelIndex &, const QModelIndex &)"), index, index)
        return True


    def getData(self, index):
        row = index.row()
        return self._data[row]


    def newItem(self, existing_label=None):
        """
        Add an item in the table view
        """
        if not self.mdl.getScalarLabelsList():
            title = self.tr("Warning")
            msg   = self.tr("There is no user scalar.\n"\
                            "Please define a user scalar.")
            QMessageBox.warning(self.parent, title, msg)
            return
        row = self.rowCount()
        if existing_label == None:
            label = self.mdl.addVariance()
        else:
            label = self.mdl.addVariance(existing_label)
        var = self.mdl.getScalarVariance(label)
        if var in ("", "no variance", "no_variance"):
            var = "no"
        scalar = [label, var]

        self.setRowCount(row+1)
        self._data.append(scalar)


    def getItem(self, row):
        """
        Return the values for an item.
        """
        [label, var] = self._data[row]
        return label, var


    def deleteItem(self, row):
        """
        Delete the row in the model.
        """
        log.debug("deleteItem row = %i " % row)

        del self._data[row]
        row = self.rowCount()
        self.setRowCount(row-1)


#-------------------------------------------------------------------------------
# Main class
#-------------------------------------------------------------------------------

class DefineUserScalarsView(QWidget, Ui_DefineUserScalarsForm):
    """
    """
    def __init__(self, parent, case, stbar):
        """
        Constructor
        """
        QWidget.__init__(self, parent)

        Ui_DefineUserScalarsForm.__init__(self)
        self.setupUi(self)

        self.case = case

        self.case.undoStopGlobal()

        self.mdl = DefineUserScalarsModel(self.case)

        # tableView
        self.modelScalars = StandardItemModelScalars(self, self.mdl)
        self.modelVariance = StandardItemModelVariance(self, self.mdl)
        self.tableScalars.horizontalHeader().setResizeMode(QHeaderView.Stretch)
        self.tableVariance.horizontalHeader().setResizeMode(QHeaderView.Stretch)

        # Delegates
        delegateLabel        = LabelDelegate(self.tableScalars)
        delegateVarianceName = VarianceNameDelegate(self.tableVariance)
        delegateVariance     = VarianceDelegate(self.tableVariance)

        self.tableScalars.setItemDelegateForColumn(0, delegateLabel)
        self.tableVariance.setItemDelegateForColumn(0, delegateVarianceName)
        self.tableVariance.setItemDelegateForColumn(1, delegateVariance)

        # Connections
        self.connect(self.pushButtonNew,       SIGNAL("clicked()"), self.slotAddScalar)
        self.connect(self.pushButtonDelete,    SIGNAL("clicked()"), self.slotDeleteScalar)
        self.connect(self.modelScalars,        SIGNAL("dataChanged(const QModelIndex &, const QModelIndex &)"), self.dataChanged)
        self.connect(self.pushButtonVarNew,    SIGNAL("clicked()"), self.slotAddVariance)
        self.connect(self.pushButtonVarDelete, SIGNAL("clicked()"), self.slotDeleteVariance)
        self.connect(self.modelVariance,       SIGNAL("dataChanged(const QModelIndex &, const QModelIndex &)"), self.dataChanged)

        # widget initialization
        self.tableScalars.reset()
        self.modelScalars = StandardItemModelScalars(self, self.mdl)
        self.tableScalars.setModel(self.modelScalars)

        self.tableVariance.reset()
        self.modelVariance = StandardItemModelVariance(self, self.mdl)
        self.tableVariance.setModel(self.modelVariance)

        l1 = self.mdl.getScalarLabelsList()
        for s in self.mdl.getScalarsVarianceList():
            if s in l1: l1.remove(s)
        for label in l1:
            self.modelScalars.newItem(label)
        for label in self.mdl.getScalarsVarianceList():
            self.modelVariance.newItem(label)

        self.case.undoStartGlobal()


    @pyqtSignature("")
    def slotAddScalar(self):
        """
        Add a new item in the table when the 'Create' button is pushed.
        """
        self.tableScalars.clearSelection()
        self.modelScalars.newItem()


    @pyqtSignature("")
    def slotDeleteScalar(self):
        """
        Just delete the current selected entries from the table and
        of course from the XML file.
        """
        lst = []
        for index in self.tableScalars.selectionModel().selectedRows():
            row = index.row()
            lst.append(row)

        lst.sort()
        lst.reverse()

        for row in lst:
            label = self.modelScalars.getItem(row)
            if self.mdl.getScalarType(label) == 'user':
                self.mdl.deleteScalar(label)
                self.modelScalars.deleteItem(row)
            row_var = self.modelVariance.rowCount()
            del_var = []
            for r in range(row_var):
                if label == self.modelVariance.getItem(r)[1]:
                    del_var.append(self.modelVariance.getItem(r)[0])
            for var in del_var:
                tot_row = self.modelVariance.rowCount()
                del_stat = 0
                for rr in range(tot_row):
                    if del_stat == 0:
                        if var == self.modelVariance.getItem(rr)[0]:
                            del_stat=1
                            self.modelVariance.deleteItem(rr)

        self.tableScalars.clearSelection()


    @pyqtSignature("")
    def slotAddVariance(self):
        """
        Add a new item in the table when the 'Create' button is pushed.
        """
        self.tableVariance.clearSelection()
        self.modelVariance.newItem()


    @pyqtSignature("")
    def slotDeleteVariance(self):
        """
        Just delete the current selected entries from the table and
        of course from the XML file.
        """
        lst = []
        for index in self.tableVariance.selectionModel().selectedRows():
            row = index.row()
            lst.append(row)

        lst.sort()
        lst.reverse()

        for row in lst:
            label = self.modelVariance.getItem(row)[0]
            self.mdl.deleteScalar(label)
            self.modelVariance.deleteItem(row)

        self.tableVariance.clearSelection()


    @pyqtSignature("const QModelIndex &, const QModelIndex &")
    def dataChanged(self, topLeft, bottomRight):
        for row in range(topLeft.row(), bottomRight.row()+1):
            self.tableView.resizeRowToContents(row)
        for col in range(topLeft.column(), bottomRight.column()+1):
            self.tableView.resizeColumnToContents(col)


    def tr(self, text):
        """
        Translation
        """
        return text

#-------------------------------------------------------------------------------
# End
#-------------------------------------------------------------------------------
