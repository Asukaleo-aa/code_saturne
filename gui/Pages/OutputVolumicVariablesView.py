# -*- coding: utf-8 -*-

#-------------------------------------------------------------------------------

# This file is part of Code_Saturne, a general-purpose CFD tool.
#
# Copyright (C) 1998-2013 EDF S.A.
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
This module contains the following classes and function:
- OutputVolumicVariablesView
"""

#-------------------------------------------------------------------------------
# Library modules import
#-------------------------------------------------------------------------------

import string, logging

#-------------------------------------------------------------------------------
# Third-party modules
#-------------------------------------------------------------------------------

from PyQt4.QtCore import *
from PyQt4.QtGui  import *

#-------------------------------------------------------------------------------
# Application modules import
#-------------------------------------------------------------------------------

from Base.Common import LABEL_LENGTH_MAX
from Base.Toolbox import GuiParam
from Base.QtPage import RegExpValidator
from Pages.OutputVolumicVariablesForm import Ui_OutputVolumicVariablesForm
from Pages.OutputControlModel import OutputControlModel
from Pages.OutputVolumicVariablesModel import OutputVolumicVariablesModel
from Pages.TimeStepModel import TimeStepModel

#-------------------------------------------------------------------------------
# log config
#-------------------------------------------------------------------------------

logging.basicConfig()
log = logging.getLogger("OutputVolumicVariablesView")
log.setLevel(GuiParam.DEBUG)

#-------------------------------------------------------------------------------
#
#-------------------------------------------------------------------------------

class ProbesValidator(QRegExpValidator):
    """
    Validator for real data.
    """
    def __init__(self, parent, xml_model):
        """
        Initialization for validator
        """
        regExp = QRegExp("^[0-9 ]*$")
        super(ProbesValidator, self).__init__(regExp, parent)
        self.parent = parent
        self.mdl = xml_model
        self.state = QValidator.Invalid


    def validate(self, string, pos):
        """
        Validation method.

        QValidator.Invalid       0  The string is clearly invalid.
        QValidator.Intermediate  1  The string is a plausible intermediate value during editing.
        QValidator.Acceptable    2  The string is acceptable as a final result; i.e. it is valid.
        """
        state = QRegExpValidator.validate(self, string, pos)[0]

        valid = True
        for probe in str(string).split():
            if probe not in self.mdl.getVariableProbeList():
                valid = False

        if state == QValidator.Acceptable:
            if not valid:
                state = QValidator.Intermediate

        palette = self.parent.palette()

        if state == QValidator.Intermediate:
            palette.setColor(QPalette.Text, QColor("red"))
            self.parent.setPalette(palette)
        else:
            palette.setColor(QPalette.Text, QColor("black"))
            self.parent.setPalette(palette)

        self.state = state

        return (state, pos)

#-------------------------------------------------------------------------------
#
#-------------------------------------------------------------------------------

class ProbesDelegate(QItemDelegate):
    """
    """
    def __init__(self, parent=None, xml_model=None):
        super(ProbesDelegate, self).__init__(parent)
        self.parent = parent
        self.mdl = xml_model


    def createEditor(self, parent, option, index):
        editor = QLineEdit(parent)
        #regExp = QRegExp("^all[ ]*$|^[0-9\ ]*$")
        #validator =  QRegExpValidator(regExp, editor)
        validator = ProbesValidator(editor, self.mdl)
        editor.setValidator(validator)
        #editor.installEventFilter(self)
        return editor


    def setEditorData(self, lineEdit, index):
        value = index.model().data(index, Qt.DisplayRole).toString()
        lineEdit.setText(value)


    def setModelData(self, editor, model, index):
        value = editor.text()
        if editor.validator().state == QValidator.Acceptable:
            selectionModel = self.parent.selectionModel()
            for idx in selectionModel.selectedIndexes():
                if idx.column() == index.column():
                    model.setData(idx, QVariant(value))

#-------------------------------------------------------------------------------
#
#-------------------------------------------------------------------------------

class LabelDelegate(QItemDelegate):
    """
    """
    def __init__(self, parent=None, xml_model=None):
        super(LabelDelegate, self).__init__(parent)
        self.parent = parent
        self.mdl = xml_model


    def createEditor(self, parent, option, index):
        editor = QLineEdit(parent)
        rx = "[\-_A-Za-z0-9]{1," + str(LABEL_LENGTH_MAX) + "}"
        self.regExp = QRegExp(rx)
        v =  RegExpValidator(editor, self.regExp)
        editor.setValidator(v)
        #editor.installEventFilter(self)
        return editor


    def setEditorData(self, editor, index):
        v = index.model().data(index, Qt.DisplayRole).toString()
        self.p_value = str(v)
        editor.setText(v)

    def setModelData(self, editor, model, index):
        if not editor.isModified():
            return

        if editor.validator().state == QValidator.Acceptable:
            p_value = str(editor.text())

            if p_value in self.mdl.getLabelsList():
                default           = {}
                default['label']  = self.p_value
                default['list']   = self.mdl.getLabelsList()
                default['regexp'] = self.regExp
                log.debug("setModelData-> default = %s" % default)

                from Pages.VerifyExistenceLabelDialogView import VerifyExistenceLabelDialogView
                dialog = VerifyExistenceLabelDialogView(self.parent, default)
                if dialog.exec_():
                    result = dialog.get_result()
                    p_value = result['label']
                    log.debug("setModelData-> result = %s" % result)
                else:
                    p_value = self.p_value

            model.setData(index, QVariant(QString(p_value)), Qt.DisplayRole)

#-------------------------------------------------------------------------------
# StandarItemModelOutput class
#-------------------------------------------------------------------------------

class VolumicOutputStandardItemModel(QStandardItemModel):

    def __init__(self, parent, case, mdl):
        """
        """
        QStandardItemModel.__init__(self)

        self.parent = parent
        self.case   = case
        self.mdl    = mdl

        #self.list_turb_probe = self.mdl.getVariableProbeList()

        self.setColumnCount(4)
        self.dataLabel    = []
        self.dataPrinting = []
        self.dataPost     = []
        self.dataProbe    = []
        self.disabledItem = []
        self.populateModel()


    def populateModel(self):
        """Data initialization"""
        for name in self.mdl.list_name:
            # row number
            row = self.rowCount()
            self.setRowCount(row + 1)

            # XML Model data
            label = self.mdl.dicoLabelName[name]
            printing = self.mdl.getPrintingStatus(label)

            if OutputControlModel(self.case).getAssociatedWriterIdList("-1") == []:
                post = "off"
                self.mdl.setPostStatus(label, post)
                self.disabledItem.append((row, 2))
            else:
                 post = self.mdl.getPostStatus(label)

            if TimeStepModel(self.case).getTimePassing() in (0, 1):
                if name == 'local_time_step':
                    self.disabledItem.append((row, 2))
                    self.disabledItem.append((row, 3))

            if not self.mdl.getVariableProbeList():
                self.disabledItem.append((row, 3))

            listProbe = self.mdl.getProbesList(label)
            if listProbe:
                probes = string.join(listProbe," ")
            else:
                probes = ""

            # StandardItemModel data
            self.dataLabel.append(label)
            self.dataPrinting.append(printing)
            self.dataPost.append(post)
            self.dataProbe.append(probes)

        # Initialize the flags
        for row in range(self.rowCount()):
            for column in range(self.columnCount()):
                if column == 0 or column == 3:
                    role = Qt.DisplayRole
                else:
                    role = Qt.CheckStateRole
                index = self.index(row, column)
                value = self.data(index, role)
                self.setData(index, value)


    def data(self, index, role):
        if not index.isValid():
            return QVariant()

        # ToolTips
        if role == Qt.ToolTipRole:
            if index.column() == 0:
                return QVariant(self.tr("Code_Saturne keyword: NOMVAR"))
            elif index.column() == 1:
                return QVariant(self.tr("Code_Saturne key word: ILISVR"))
            elif index.column() == 2:
                return QVariant(self.tr("Code_Saturne key word: ICHRVR"))
            elif index.column() == 3:
                return QVariant(self.tr("Code_Saturne key word: IHISVR"))

        # StatusTips
        if role == Qt.StatusTipRole:
            if index.column() == 0:
                return QVariant(self.tr("Variable/Scalar name =< 32 characters"))
            elif index.column() == 1:
                return QVariant(self.tr("Print in listing"))
            elif index.column() == 2:
                return QVariant(self.tr("Post-processing"))
            elif index.column() == 3:
                return QVariant(self.tr("Enter Probes number"))

        # Display
        if role == Qt.DisplayRole:
            row = index.row()
            if index.column() == 0:
                return QVariant(self.dataLabel[row])
            elif index.column() == 3:
                return QVariant(self.dataProbe[row])
            else:
                return QVariant()

        # CheckState
        if role == Qt.CheckStateRole:
            row = index.row()
            if index.column() == 1:
                value = self.dataPrinting[row]
                if value == 'on':
                    return QVariant(Qt.Checked)
                else:
                    return QVariant(Qt.Unchecked)

            elif index.column() == 2:
                value = self.dataPost[row]
                if value == 'on':
                    return QVariant(Qt.Checked)
                else:
                    return QVariant(Qt.Unchecked)

        return QVariant()


    def flags(self, index):
        if not index.isValid():
            return Qt.ItemIsEnabled

        # disable item
        if (index.row(), index.column()) in self.disabledItem:
            return Qt.ItemIsEnabled

        if index.column() == 1 or index.column() == 2:
            return Qt.ItemIsEnabled | Qt.ItemIsSelectable | Qt.ItemIsUserCheckable
        else:
            return Qt.ItemIsEnabled | Qt.ItemIsSelectable | Qt.ItemIsEditable


    def headerData(self, section, orientation, role):
        if orientation == Qt.Horizontal and role == Qt.DisplayRole:
            if section == 0:
                return QVariant(self.tr("Name"))
            elif section == 1:
                return QVariant(self.tr("Print in\nlisting"))
            elif section == 2:
                return QVariant(self.tr("Post-\nprocessing"))
            elif section == 3:
                return QVariant(self.tr("Probes"))
        return QVariant()


    def setData(self, index, value, role=None):
        row = index.row()
        if index.column() == 0:
            label = str(value.toString())
            if label == "": label = self.dataLabel[row]
            self.mdl.setVariableLabel(self.dataLabel[row], label)
            self.dataLabel[row] = label

        elif index.column() == 1:
            v, ok = value.toInt()
            if v == Qt.Checked:
                self.dataPrinting[row] = "on"
            else:
                self.dataPrinting[row] = "off"
            self.mdl.setPrintingStatus(self.dataLabel[row], self.dataPrinting[row])

        elif index.column() == 2:
            v, ok = value.toInt()
            if v == Qt.Checked:
                self.dataPost[row] = "on"
            else:
                self.dataPost[row] = "off"

            if OutputControlModel(self.case).getAssociatedWriterIdList("-1") == []:
                self.dataPost[row] = "off"

            self.mdl.setPostStatus(self.dataLabel[row], self.dataPost[row])

        elif index.column() == 3:
            probes = str(value.toString())
            self.dataProbe[row] = probes
            #print(probes)
            self.mdl.updateProbes(self.dataLabel[row], probes)

        self.emit(SIGNAL("dataChanged(const QModelIndex &, const QModelIndex &)"), index, index)
        return True

# TODO: VolumicOutputStandardItemModel (selectAllProbe cancelAllProbe methods)
##     def selectAll(self, event= None):
##         self.probe.set(string.join(self.list_turb_probe))
##         self.getProbe()


##     def cancelAll(self, event= None):
##         self.probe.set("")
##         self.getProbe()


#-------------------------------------------------------------------------------
# Main class
#-------------------------------------------------------------------------------

class OutputVolumicVariablesView(QWidget, Ui_OutputVolumicVariablesForm):
    """
    """
    def __init__(self, parent, case):
        """
        Constructor
        """
        QWidget.__init__(self, parent)

        Ui_OutputVolumicVariablesForm.__init__(self)
        self.setupUi(self)

        self.case = case
        self.case.undoStopGlobal()
        self.info_turb_name = []
        self.mdl = OutputVolumicVariablesModel(self.case)

        self.modelOutput = VolumicOutputStandardItemModel(parent, self.case, self.mdl)
        self.tableViewOutput.setModel(self.modelOutput)
        self.tableViewOutput.setAlternatingRowColors(True)
        self.tableViewOutput.resizeColumnToContents(0)
        self.tableViewOutput.resizeRowsToContents()
        self.tableViewOutput.setSelectionBehavior(QAbstractItemView.SelectItems)
        self.tableViewOutput.setSelectionMode(QAbstractItemView.ExtendedSelection)
        self.tableViewOutput.setEditTriggers(QAbstractItemView.DoubleClicked)
        self.tableViewOutput.horizontalHeader().setResizeMode(QHeaderView.Stretch)

        labelDelegate = LabelDelegate(self.tableViewOutput, self.mdl)
        self.tableViewOutput.setItemDelegateForColumn(0, labelDelegate)

        probesDelegate = ProbesDelegate(self.tableViewOutput, self.mdl)
        self.tableViewOutput.setItemDelegateForColumn(3, probesDelegate)

        self.case.undoStartGlobal()


    def tr(self, text):
        """
        Translation
        """
        return text


#-------------------------------------------------------------------------------
# Testing part
#-------------------------------------------------------------------------------


if __name__ == "__main__":
    pass


#-------------------------------------------------------------------------------
# End
#-------------------------------------------------------------------------------
