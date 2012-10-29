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
This module contains the following classes:
- StandardItemModelCoals
- StandardItemModelClasses
- StandardItemModelOxidant
- StandardItemModelRefusal
- CoalCombustionView
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

from CoalCombustionForm import Ui_CoalCombustionForm

from Base.Toolbox import GuiParam
from Base.Common import LABEL_LENGTH_MAX
from Base.QtPage import ComboModel, DoubleValidator, RegExpValidator, setGreenColor

from Pages.Boundary import Boundary
from Pages.LocalizationModel import LocalizationModel
from Pages.CoalCombustionModel import CoalCombustionModel

#-------------------------------------------------------------------------------
# log config
#-------------------------------------------------------------------------------

logging.basicConfig()
log = logging.getLogger("CoalCombustionView")
log.setLevel(GuiParam.DEBUG)

#-------------------------------------------------------------------------------
# Line edit delegate for the label fel
#-------------------------------------------------------------------------------

class LabelFuelDelegate(QItemDelegate):
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
        rx = "[_A-Za-z0-9 \(\)]{1," + str(LABEL_LENGTH_MAX-1) + "}"
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

            if new_plabel in model.mdl.getLabelIdList():
                default = {}
                default['label']  = self.old_plabel
                default['list']   = model.mdl.getLabelIdList()
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
# Combo box delegate for the fuel type
#-------------------------------------------------------------------------------

class TypeFuelDelegate(QItemDelegate):
    """
    Use of a combo box in the table.
    """
    def __init__(self, parent=None, xml_model=None):
        super(TypeFuelDelegate, self).__init__(parent)
        self.parent = parent
        self.mdl = xml_model


    def createEditor(self, parent, option, index):
        editor = QComboBox(parent)
        editor.addItem(QString("biomass"))
        editor.addItem(QString("coal"))
        editor.installEventFilter(self)
        return editor


    def setEditorData(self, comboBox, index):
        dico = {"biomass": 0, "coal": 1}
        row = index.row()
        string = index.model().dataCoals[row]['type']
        idx = dico[string]
        comboBox.setCurrentIndex(idx)


    def setModelData(self, comboBox, model, index):
        value = comboBox.currentText()
        selectionModel = self.parent.selectionModel()
        for idx in selectionModel.selectedIndexes():
            if idx.column() == index.column():
                model.setData(idx, QVariant(value), Qt.DisplayRole)


    def paint(self, painter, option, index):
        row = index.row()
        fueltype = index.model().dataCoals[row]['type']
        isValid = fueltype != None and fueltype != ''

        if isValid:
            QItemDelegate.paint(self, painter, option, index)
        else:
            painter.save()
            # set background color
            if option.state & QStyle.State_Selected:
                painter.setBrush(QBrush(Qt.darkRed))
            else:
                painter.setBrush(QBrush(Qt.red))
            # set text color
            painter.setPen(QPen(Qt.NoPen))
            painter.drawRect(option.rect)
            painter.setPen(QPen(Qt.black))
            value = index.data(Qt.DisplayRole)
            if value.isValid():
                text = value.toString()
                painter.drawText(option.rect, Qt.AlignLeft, text)
            painter.restore()

#-------------------------------------------------------------------------------
# Delegate for diameter
#-------------------------------------------------------------------------------

class DiameterDelegate(QItemDelegate):
    def __init__(self, parent):
        super(DiameterDelegate, self).__init__(parent)
        self.parent = parent


    def createEditor(self, parent, option, index):
        editor = QLineEdit(parent)
        v = DoubleValidator(editor, min=0.)
        v.setExclusiveMin()
        editor.setValidator(v)
        return editor


    def setEditorData(self, editor, index):
        value = index.model().data(index, Qt.DisplayRole).toString()
        editor.setText(value)


    def setModelData(self, editor, model, index):
        if editor.validator().state == QValidator.Acceptable:
            value, ok = editor.text().toDouble()
            model.setData(index, QVariant(value), Qt.DisplayRole)

#-------------------------------------------------------------------------------
# Delegate for refusal
#-------------------------------------------------------------------------------

class RefusalDelegate(QItemDelegate):
    def __init__(self, parent):
        super(RefusalDelegate, self).__init__(parent)
        self.parent = parent


    def createEditor(self, parent, option, index):
        editor = QLineEdit(parent)
        v = DoubleValidator(editor, min=0.)
        v.setExclusiveMin()
        editor.setValidator(v)
        return editor


    def setEditorData(self, editor, index):
        value = index.model().data(index, Qt.DisplayRole).toString()
        editor.setText(value)


    def setModelData(self, editor, model, index):
        if editor.validator().state == QValidator.Acceptable:
            value, ok = editor.text().toDouble()
            model.setData(index, QVariant(value), Qt.DisplayRole)

#-------------------------------------------------------------------------------
# Delegate for oxidant composition
#-------------------------------------------------------------------------------

class OxidantDelegate(QItemDelegate):
    def __init__(self, parent):
        super(OxidantDelegate, self).__init__(parent)
        self.parent = parent


    def createEditor(self, parent, option, index):
        editor = QLineEdit(parent)
        v = DoubleValidator(editor, min=0.)
        editor.setValidator(v)
        #editor.installEventFilter(self)
        return editor


    def setEditorData(self, editor, index):
        value = index.model().data(index, Qt.DisplayRole).toString()
        editor.setText(value)


    def setModelData(self, editor, model, index):
        if editor.validator().state == QValidator.Acceptable:
            value, ok = editor.text().toDouble()
            model.setData(index, QVariant(value), Qt.DisplayRole)

#-------------------------------------------------------------------------------
# StandarItemModel for Coals
#-------------------------------------------------------------------------------

class StandardItemModelCoals(QStandardItemModel):
    def __init__(self, mdl):
        """
        """
        QStandardItemModel.__init__(self)
        self.headers = [self.tr("Name"), self.tr("Type")]
        self.setColumnCount(len(self.headers))
        self.mdl = mdl
        self.dataCoals = []
        self.defaultItem = []
        self.populateModel()


    def populateModel(self):
        self.dicoV2M= {"biomass": 'biomass',
                       "coal" : 'coal'}
        self.dicoM2V= {"biomass" : 'biomass',
                       "coal" : 'coal'}
        for id in self.mdl.getFuelIdList():
            row = self.rowCount()
            self.setRowCount(row + 1)

            dico  = {}
            dico['name'] = self.mdl.getFuelLabel(id)
            dico['type'] = self.mdl.getFuelType(id)

            self.dataCoals.append(dico)
            if int(id) < 0:
                self.defaultItem.append(row)
            log.debug("populateModel-> dataSolver = %s" % dico)


    def data(self, index, role):
        if not index.isValid():
            return QVariant()

        if role == Qt.DisplayRole:
            row = index.row()
            col = index.column()
            dico = self.dataCoals[row]

            if index.column() == 0:
                return QVariant(dico['name'])
            elif index.column() == 1:
                return QVariant(self.dicoM2V[dico['type']])
            else:
                return QVariant()

        elif role == Qt.TextAlignmentRole:
            return QVariant(Qt.AlignCenter)

        return QVariant()


    def flags(self, index):
        if not index.isValid():
            return Qt.ItemIsEnabled
        else:
            return Qt.ItemIsEnabled | Qt.ItemIsSelectable | Qt.ItemIsEditable


    def headerData(self, section, orientation, role):
        if orientation == Qt.Horizontal and role == Qt.DisplayRole:
            return QVariant(self.headers[section])
        return QVariant()


    def setData(self, index, value, role):
        # Update the row in the table
        row = index.row()
        col = index.column()

        # Label
        if col == 0:
            old_plabel = self.dataCoals[row]['name']
            new_plabel = str(value.toString())
            self.dataCoals[row]['name'] = new_plabel
            self.mdl.setFuelLabel(row + 1, new_plabel)

        elif col == 1:
            self.dataCoals[row]['type'] = self.dicoV2M[str(value.toString())]
            self.mdl.setFuelType(row + 1, self.dataCoals[row]['type'])

        self.emit(SIGNAL("dataChanged(const QModelIndex &, const QModelIndex &)"), index, index)
        return True


    def addItem(self, name = None, fuel_type = None):
        """
        Add a row in the table.
        """
        dico = {}
        if (name != None and fuel_type != None):
            dico['name'] = name
            dico['type'] = fuel_type
        else:
            self.mdl.createCoal()
            number = self.mdl.getCoalNumber()
            dico['name'] = self.mdl.getFuelLabel(number)
            dico['type'] = self.mdl.getFuelType(number)
        self.dataCoals.append(dico)

        row = self.rowCount()
        self.setRowCount(row+1)


    def getItem(self, row):
        """
        Returns the name of the fuel file.
        """
        return self.dataCoals[row]


    def deleteItem(self, row):
        """
        Delete the row in the model
        """
        del self.dataCoals[row]
        row = self.rowCount()
        self.setRowCount(row-1)


    def deleteAll(self):
        """
        Delete all the rows in the model
        """
        self.dataCoals = []
        self.setRowCount(0)


#-------------------------------------------------------------------------------
# StandarItemModel for Coal Classes
#-------------------------------------------------------------------------------

class StandardItemModelClasses(QStandardItemModel):
    def __init__(self, model, fuel):
        """
        """
        QStandardItemModel.__init__(self)
        self.model = model
        self.fuel = fuel
        diameter_type = self.model.getDiameterType(self.fuel)

        if diameter_type == 'automatic' :
            self.headers = [self.tr("class number"),
                            self.tr("Initial diameter (m)")]
        elif diameter_type == 'rosin-rammler_law':
            self.headers = [self.tr("class number"),
                            self.tr("Mass percent")]

        self.setColumnCount(len(self.headers))
        self.dataClasses = []


    def data(self, index, role):
        if not index.isValid():
            return QVariant()
        if role == Qt.DisplayRole:
            return QVariant(self.dataClasses[index.row()][index.column()])
        return QVariant()


    def flags(self, index):
        if not index.isValid():
            return Qt.ItemIsEnabled
        elif index.column() == 1:
            return Qt.ItemIsEnabled | Qt.ItemIsSelectable | Qt.ItemIsEditable
        else:
            return Qt.ItemIsEnabled | Qt.ItemIsSelectable


    def headerData(self, section, orientation, role):
        if orientation == Qt.Horizontal and role == Qt.DisplayRole:
            return QVariant(self.headers[section])
        return QVariant()


    def setData(self, index, value, role):
        if not index.isValid():
            return Qt.ItemIsEnabled
        row = index.row()
        col = index.column()
        ClassId = row + 1

        if col == 1:
            newDiameter, ok = value.toDouble()
            self.dataClasses[row][col] = newDiameter

            diameter_type = self.model.getDiameterType(self.fuel)
            if diameter_type == 'automatic' :
                self.model.setDiameter(self.fuel, ClassId, newDiameter)
            elif diameter_type == 'rosin-rammler_law':
                self.model.setMassPercent(self.fuel, ClassId, newDiameter)

        self.emit(SIGNAL("dataChanged(const QModelIndex &, const QModelIndex &)"), index, index)
        return True


    def addItem(self, num, diameter):
        """
        Add a row in the table.
        """
        label = "Class " + str(num)
        item = [label, diameter]
        self.dataClasses.append(item)
        row = self.rowCount()
        self.setRowCount(row+1)


    def getItem(self, row):
        return self.dataClasses[row]


    def deleteRow(self, row):
        """
        Delete the row in the model
        """
        del self.dataClasses[row]
        row = self.rowCount()
        self.setRowCount(row-1)


    def deleteAll(self):
        """
        Delete all the rows in the model
        """
        self.dataClasses = []
        self.setRowCount(0)

#-------------------------------------------------------------------------------
# StandarItemModel for Oxidant
#-------------------------------------------------------------------------------

class StandardItemModelOxidant(QStandardItemModel):
    def __init__(self, model):
        """
        """
        QStandardItemModel.__init__(self)
        self.headers = [self.tr("Oxidant\nnumber"),
                        self.tr("     O2      "),
                        self.tr("     N2      "),
                        self.tr("     H2O     "),
                        self.tr("     CO2     ")]
        self.setColumnCount(len(self.headers))
        self.dataClasses = []
        self.model = model


    def data(self, index, role):
        if not index.isValid():
            return QVariant()
        if role == Qt.DisplayRole:
            return QVariant(self.dataClasses[index.row()][index.column()])
        return QVariant()


    def flags(self, index):
        if not index.isValid():
            return Qt.ItemIsEnabled
        elif index.column() != 0:
            return Qt.ItemIsEnabled | Qt.ItemIsSelectable | Qt.ItemIsEditable
        else:
            return Qt.ItemIsEnabled | Qt.ItemIsSelectable


    def headerData(self, section, orientation, role):
        if orientation == Qt.Horizontal and role == Qt.DisplayRole:
            return QVariant(self.headers[section])
        return QVariant()


    def setData(self, index, value, role):
        if not index.isValid():
            return Qt.ItemIsEnabled
        row = index.row()
        col = index.column()
        v, ok = value.toDouble()
        self.dataClasses[row][col] = v
        oxId = row + 1

        if col == 1:
            self.model.setElementComposition(oxId, "O2", v)
        elif col == 2:
            self.model.setElementComposition(oxId, "N2", v)
        elif col == 3:
            self.model.setElementComposition(oxId, "H2O", v)
        elif col == 4:
            self.model.setElementComposition(oxId, "CO2", v)

        self.emit(SIGNAL("dataChanged(const QModelIndex &, const QModelIndex &)"), index, index)
        return True


    def addItem(self, num):
        """
        Add a row in the table.
        """
        label = str(num)
        O2  = self.model.getElementComposition(num, "O2")
        N2  = self.model.getElementComposition(num, "N2")
        H2O = self.model.getElementComposition(num, "H2O")
        CO2 = self.model.getElementComposition(num, "CO2")
        item = [label, O2, N2, H2O, CO2]
        self.dataClasses.append(item)
        row = self.rowCount()
        self.setRowCount(row + 1)


    def getItem(self, row):
        return self.dataClasses[row]


    def deleteRow(self, row):
        """
        Delete the row in the model
        """
        del self.dataClasses[row]
        row = self.rowCount()
        self.setRowCount(row-1)
        self.model.deleteOxidant(row+1)

    def deleteAll(self):
        """
        Delete all the rows in the model
        """
        self.dataClasses = []
        self.setRowCount(0)


#-------------------------------------------------------------------------------
# StandarItemModel for Refusal
#-------------------------------------------------------------------------------

class StandardItemModelRefusal(QStandardItemModel):
    def __init__(self, model, fuel):
        """
        """
        QStandardItemModel.__init__(self)
        self.headers = [self.tr("Refusal"),
                        self.tr("diameter (m)"),
                        self.tr("value")]
        self.setColumnCount(len(self.headers))
        self.dataClasses = []
        self.model = model
        self.fuel = fuel


    def data(self, index, role):
        if not index.isValid():
            return QVariant()
        if role == Qt.DisplayRole:
            return QVariant(self.dataClasses[index.row()][index.column()])
        return QVariant()


    def flags(self, index):
        if not index.isValid():
            return Qt.ItemIsEnabled
        elif index.column() != 0:
            return Qt.ItemIsEnabled | Qt.ItemIsSelectable | Qt.ItemIsEditable
        else:
            return Qt.ItemIsEnabled | Qt.ItemIsSelectable


    def headerData(self, section, orientation, role):
        if orientation == Qt.Horizontal and role == Qt.DisplayRole:
            return QVariant(self.headers[section])
        return QVariant()


    def setData(self, index, value, role):
        if not index.isValid():
            return Qt.ItemIsEnabled
        row = index.row()
        col = index.column()
        v, ok = value.toDouble()
        self.dataClasses[row][col] = v

        if col == 1:
            self.model.setRefusalDiameter(self.fuel, row+1, v)
        elif col == 2:
            self.model.setRefusalValue(self.fuel, row+1, v)

        self.emit(SIGNAL("dataChanged(const QModelIndex &, const QModelIndex &)"), index, index)
        return True


    def addItem(self, num, item):
        """
        Add a row in the table.
        """
        label = str(num)
        self.dataClasses.append(item)
        row = self.rowCount()
        self.setRowCount(row+1)


    def getItem(self, row):
        return self.dataClasses[row]


    def deleteRow(self, row):
        """
        Delete the row in the model
        """
        del self.dataClasses[row]
        row = self.rowCount()
        self.setRowCount(row-1)
        self.model.deleteRefusal(self.fuel, row+1)

    def deleteAll(self):
        """
        Delete all the rows in the model
        """
        self.dataClasses = []
        self.setRowCount(0)

#-------------------------------------------------------------------------------
# Main class
#-------------------------------------------------------------------------------

class CoalCombustionView(QWidget, Ui_CoalCombustionForm):
    """
    """
    def __init__(self, parent, case, stbar):
        """
        Constructor
        """
        QWidget.__init__(self, parent)

        Ui_CoalCombustionForm.__init__(self)
        self.setupUi(self)

        self.case = case
        self.case.undoStopGlobal()

        self.stbar = stbar

        self.model = CoalCombustionModel(self.case)

        # widgets layout.
        self.fuel = 1

        # Models
        # ------
        self.modelCoals = StandardItemModelCoals(self.model)
        self.treeViewCoals.setModel(self.modelCoals)
        delegate_label_fuel = LabelFuelDelegate(self.treeViewCoals)
        delegate_type       = TypeFuelDelegate(self.treeViewCoals, self.model)
        self.treeViewCoals.setItemDelegateForColumn(0, delegate_label_fuel)
        self.treeViewCoals.setItemDelegateForColumn(1, delegate_type)

        self.modelClasses = StandardItemModelClasses(self.model, self.fuel)
        self.treeViewClasses.setModel(self.modelClasses)
        self.treeViewClasses.resizeColumnToContents(0)
        self.treeViewClasses.resizeColumnToContents(1)

        self.modelRefusal = StandardItemModelRefusal(self.model, self.fuel)
        self.treeViewRefusal.setModel(self.modelRefusal)
        self.treeViewRefusal.resizeColumnToContents(0)
        self.treeViewRefusal.resizeColumnToContents(1)
        self.treeViewRefusal.resizeColumnToContents(2)

        self.modelOxidants = StandardItemModelOxidant(self.model)
        self.tableViewOxidants.setModel(self.modelOxidants)
        self.tableViewOxidants.resizeColumnsToContents()
        self.tableViewOxidants.resizeRowsToContents()

        delegateDiameter = DiameterDelegate(self.treeViewClasses)
        self.treeViewClasses.setItemDelegateForColumn(1, delegateDiameter)
        delegateRefusal = RefusalDelegate(self.treeViewRefusal)
        self.treeViewRefusal.setItemDelegate(delegateRefusal)
        delegateOxidant = OxidantDelegate(self.tableViewOxidants)
        self.tableViewOxidants.setItemDelegate(delegateOxidant)

        # Combo box
        # ---------
        self.modelPCI = ComboModel(self.comboBoxPCIList,3,1)
        self.modelPCI.addItem(self.tr("LHV"), "LHV")
        self.modelPCI.addItem(self.tr("HHV"), "HHV")
        self.modelPCI.addItem(self.tr("IGT correlation"), "IGT_correlation")

        self.modelPCIType = ComboModel(self.comboBoxPCIType,3,1)
        self.modelPCIType.addItem(self.tr("dry basis"),    "dry_basis")
        self.modelPCIType.addItem(self.tr("dry ash free"), "dry_ash_free")
        self.modelPCIType.addItem(self.tr("as received"),  "as_received")

        self.modelY1Y2 = ComboModel(self.comboBoxY1Y2,3,1)
        self.modelY1Y2.addItem(self.tr("user define"),       "user_define")
        self.modelY1Y2.addItem(self.tr("automatic CHONS"),   "automatic_CHONS")
        self.modelY1Y2.addItem(self.tr("automatic formula"), "automatic_formula")

        self.modelDiameter = ComboModel(self.comboBoxDiameter,2,1)
        self.modelDiameter.addItem(self.tr("user define"),       "automatic")
        self.modelDiameter.addItem(self.tr("Rosin-Rammler law"), "rosin-rammler_law")

        self.modelReactTypeO2 = ComboModel(self.comboBoxReactO2,2,1)
        self.modelReactTypeO2.addItem(self.tr("0.5"), "0.5")
        self.modelReactTypeO2.addItem(self.tr("1"),   "1")

        self.modelReactTypeCO2 = ComboModel(self.comboBoxReactCO2,2,1)
        self.modelReactTypeCO2.addItem(self.tr("0.5"), "0.5")
        self.modelReactTypeCO2.addItem(self.tr("1"),   "1")

        self.modelReactTypeH2O = ComboModel(self.comboBoxReactH2O,2,1)
        self.modelReactTypeH2O.addItem(self.tr("0.5"), "0.5")
        self.modelReactTypeH2O.addItem(self.tr("1"),   "1")

        self.modelOxidantType = ComboModel(self.comboBoxOxidant,2,1)
        self.modelOxidantType.addItem(self.tr("volumic percentage"), "volumic_percent")
        self.modelOxidantType.addItem(self.tr("molar"),              "molar")

        # Connections
        # -----------
        self.connect(self.treeViewCoals,           SIGNAL("clicked(const QModelIndex &)"), self.slotSelectCoal)
        self.connect(self.pushButtonAddCoal,       SIGNAL("clicked()"), self.slotCreateCoal)
        self.connect(self.pushButtonDeleteCoal,    SIGNAL("clicked()"), self.slotDeleteCoal)
        self.connect(self.pushButtonAddClass,      SIGNAL("clicked()"), self.slotCreateClass)
        self.connect(self.pushButtonDeleteClass,   SIGNAL("clicked()"), self.slotDeleteClass)
        self.connect(self.pushButtonAddRefusal,    SIGNAL("clicked()"), self.slotCreateRefusal)
        self.connect(self.pushButtonDeleteRefusal, SIGNAL("clicked()"), self.slotDeleteRefusal)
        self.connect(self.comboBoxDiameter,        SIGNAL("activated(const QString&)"), self.slotDiameterType)
        self.connect(self.pushButtonAddOxidant,    SIGNAL("clicked()"), self.slotCreateOxidant)
        self.connect(self.pushButtonDeleteOxidant, SIGNAL("clicked()"), self.slotDeleteOxidant)

        self.connect(self.lineEditC,               SIGNAL("textChanged(const QString &)"), self.slotCComposition)
        self.connect(self.lineEditH,               SIGNAL("textChanged(const QString &)"), self.slotHComposition)
        self.connect(self.lineEditO,               SIGNAL("textChanged(const QString &)"), self.slotOComposition)
        self.connect(self.lineEditN,               SIGNAL("textChanged(const QString &)"), self.slotNComposition)
        self.connect(self.lineEditS,               SIGNAL("textChanged(const QString &)"), self.slotSComposition)
        self.connect(self.lineEditPCI,             SIGNAL("textChanged(const QString &)"), self.slotPCI)
        self.connect(self.lineEditVolatileMatter,  SIGNAL("textChanged(const QString &)"), self.slotVolatileMatter)
        self.connect(self.lineEditMoisture,        SIGNAL("textChanged(const QString &)"), self.slotMoisture)
        self.connect(self.lineEditCp,              SIGNAL("textChanged(const QString &)"), self.slotThermalCapacity)
        self.connect(self.lineEditDensity,         SIGNAL("textChanged(const QString &)"), self.slotDensity)
        self.connect(self.comboBoxPCIList,         SIGNAL("activated(const QString&)"), self.slotPCIChoice)
        self.connect(self.comboBoxPCIType,         SIGNAL("activated(const QString&)"), self.slotPCIType)

        self.connect(self.lineEditAshesRatio,      SIGNAL("textChanged(const QString &)"), self.slotAshesRatio)
        self.connect(self.lineEditAshesEnthalpy,   SIGNAL("textChanged(const QString &)"), self.slotAshesFormingEnthalpy)
        self.connect(self.lineEditAshesCp,         SIGNAL("textChanged(const QString &)"), self.slotAshesThermalCapacity)

        self.connect(self.comboBoxY1Y2,   SIGNAL("activated(const QString&)"), self.slotY1Y2)
        self.connect(self.lineEditCoefY1, SIGNAL("textChanged(const QString &)"), self.slotY1CH)
        self.connect(self.lineEditCoefY2, SIGNAL("textChanged(const QString &)"), self.slotY2CH)
        self.connect(self.lineEditCoefA1, SIGNAL("textChanged(const QString &)"), self.slotA1CH)
        self.connect(self.lineEditCoefA2, SIGNAL("textChanged(const QString &)"), self.slotA2CH)
        self.connect(self.lineEditCoefE1, SIGNAL("textChanged(const QString &)"), self.slotE1CH)
        self.connect(self.lineEditCoefE2, SIGNAL("textChanged(const QString &)"), self.slotE2CH)

        self.connect(self.lineEditConstO2,   SIGNAL("textChanged(const QString &)"), self.slotPreExpoCstO2)
        self.connect(self.lineEditEnergyO2,  SIGNAL("textChanged(const QString &)"), self.slotActivEnergyO2)
        self.connect(self.comboBoxReactO2,   SIGNAL("activated(const QString&)"), self.slotReactTypeO2)
        self.connect(self.lineEditConstCO2,  SIGNAL("textChanged(const QString &)"), self.slotPreExpoCstCO2)
        self.connect(self.lineEditEnergyCO2, SIGNAL("textChanged(const QString &)"), self.slotActivEnergyCO2)
        self.connect(self.comboBoxReactCO2,  SIGNAL("activated(const QString&)"), self.slotReactTypeCO2)
        self.connect(self.lineEditConstH2O,  SIGNAL("textChanged(const QString &)"), self.slotPreExpoCstH2O)
        self.connect(self.lineEditEnergyH2O, SIGNAL("textChanged(const QString &)"), self.slotActivEnergyH2O)
        self.connect(self.comboBoxReactH2O,  SIGNAL("activated(const QString&)"), self.slotReactTypeH2O)
        self.connect(self.comboBoxOxidant,   SIGNAL("activated(const QString&)"), self.slotOxidantType)

        self.connect(self.lineEditQPR,                   SIGNAL("textChanged(const QString &)"), self.slotQPR)
        self.connect(self.lineEditNitrogenConcentration, SIGNAL("textChanged(const QString &)"), self.slotNitrogenConcentration)
        self.connect(self.lineEditKobayashi1,            SIGNAL("textChanged(const QString &)"), self.slotKobayashi1)
        self.connect(self.lineEditKobayashi2,            SIGNAL("textChanged(const QString &)"), self.slotKobayashi2)

        self.connect(self.checkBoxNOxFormation, SIGNAL("clicked(bool)"), self.slotNOxFormation)
        self.connect(self.checkBoxCO2Kinetics,  SIGNAL("clicked(bool)"), self.slotCO2Kinetics)
        self.connect(self.checkBoxH2OKinetics,  SIGNAL("clicked(bool)"), self.slotH2OKinetics)

        self.connect(self.tabWidget,            SIGNAL("currentChanged(int)"), self.slotchanged)

        # Validators
        # ----------
        validatorC   = DoubleValidator(self.lineEditC, min=0., max=100.)
        validatorH   = DoubleValidator(self.lineEditH, min=0., max=100.)
        validatorO   = DoubleValidator(self.lineEditO, min=0., max=100.)
        validatorN   = DoubleValidator(self.lineEditN, min=0., max=100.)
        validatorS   = DoubleValidator(self.lineEditS, min=0., max=100.)
        validatorPCI = DoubleValidator(self.lineEditPCI, min=0.)
        validatorCp  = DoubleValidator(self.lineEditCp, min=0.)
        validatorDensity = DoubleValidator(self.lineEditDensity, min=0.)
        validatorMoisture = DoubleValidator(self.lineEditMoisture, min=0., max=100.)
        validatorVolatileMatter = DoubleValidator(self.lineEditVolatileMatter, min=0., max=100.)

        validatorAshesRatio = DoubleValidator(self.lineEditAshesRatio, min=0., max=100.)
        validatorAshesEnthalpy = DoubleValidator(self.lineEditAshesEnthalpy, min=0.)
        validatorAshesCp = DoubleValidator(self.lineEditAshesCp, min=0.)

        validatorY1 = DoubleValidator(self.lineEditCoefY1, min=0.)
        validatorY2 = DoubleValidator(self.lineEditCoefY2, min=0.)
        validatorA1 = DoubleValidator(self.lineEditCoefA1, min=0.)
        validatorA2 = DoubleValidator(self.lineEditCoefA2, min=0.)
        validatorE1 = DoubleValidator(self.lineEditCoefE1, min=0.)
        validatorE2 = DoubleValidator(self.lineEditCoefE2, min=0.)

        validatorConstO2   = DoubleValidator(self.lineEditConstO2, min=0.)
        validatorEnergyO2  = DoubleValidator(self.lineEditEnergyO2, min=0.)
        validatorConstCO2  = DoubleValidator(self.lineEditConstCO2, min=0.)
        validatorEnergyCO2 = DoubleValidator(self.lineEditEnergyCO2, min=0.)
        validatorConstH2O  = DoubleValidator(self.lineEditConstH2O, min=0.)
        validatorEnergyH2O = DoubleValidator(self.lineEditEnergyH2O, min=0.)

        validatorQPR = DoubleValidator(self.lineEditQPR, min=0.)
        validatorNitrogenConcentration = DoubleValidator(self.lineEditNitrogenConcentration, min=0.)
        validatorKobayashi1 = DoubleValidator(self.lineEditKobayashi1, min=0., max = 1.)
        validatorKobayashi2 = DoubleValidator(self.lineEditKobayashi2, min=0., max = 1.)

        self.lineEditC.setValidator(validatorC)
        self.lineEditH.setValidator(validatorH)
        self.lineEditO.setValidator(validatorO)
        self.lineEditN.setValidator(validatorN)
        self.lineEditS.setValidator(validatorS)
        self.lineEditPCI.setValidator(validatorPCI)
        self.lineEditCp.setValidator(validatorCp)
        self.lineEditDensity.setValidator(validatorDensity)

        self.lineEditAshesRatio.setValidator(validatorAshesRatio)
        self.lineEditAshesEnthalpy.setValidator(validatorAshesEnthalpy)
        self.lineEditAshesCp.setValidator(validatorAshesCp)
        self.lineEditMoisture.setValidator(validatorMoisture)
        self.lineEditVolatileMatter.setValidator(validatorVolatileMatter)

        self.lineEditCoefY1.setValidator(validatorY1)
        self.lineEditCoefY2.setValidator(validatorY2)
        self.lineEditCoefA1.setValidator(validatorA1)
        self.lineEditCoefA2.setValidator(validatorA2)
        self.lineEditCoefE1.setValidator(validatorE1)
        self.lineEditCoefE2.setValidator(validatorE2)

        self.lineEditConstO2.setValidator(validatorConstO2)
        self.lineEditEnergyO2.setValidator(validatorEnergyO2)
        self.lineEditConstCO2.setValidator(validatorConstCO2)
        self.lineEditEnergyCO2.setValidator(validatorEnergyCO2)
        self.lineEditConstH2O.setValidator(validatorConstH2O)
        self.lineEditEnergyH2O.setValidator(validatorEnergyH2O)

        self.lineEditQPR.setValidator(validatorQPR)
        self.lineEditNitrogenConcentration.setValidator(validatorNitrogenConcentration)
        self.lineEditKobayashi1.setValidator(validatorKobayashi1)
        self.lineEditKobayashi2.setValidator(validatorKobayashi2)

        # Initialize widgets
        self.initializeView()

        num = self.model.getOxidantNumber()
        for index in range(0, num):
            self.modelOxidants.addItem(index + 1)

        # Update buttons
        self._updateCoalButton()
        self._updateOxidantButton()

        self.tabWidget.setCurrentIndex(self.case['current_tab'])

        self.case.undoStartGlobal()


    def _updateCoalButton(self):
        """
        control solid fuel number between 1 and 3
        """
        CoalNumber = self.model.getCoalNumber()
        self.pushButtonDeleteCoal.setEnabled(True)
        self.pushButtonAddCoal.setEnabled(True)
        if CoalNumber >= 3:
            self.pushButtonAddCoal.setDisabled(True)
        elif CoalNumber <= 1:
            self.pushButtonDeleteCoal.setDisabled(True)


    def _updateClassButton(self):
        """
        control class number between 1 and 10 for a define solid fuel
        """
        ClassNumber = self.model.getClassNumber(self.fuel)

        self.pushButtonDeleteClass.setEnabled(True)
        self.pushButtonAddClass.setEnabled(True)
        if ClassNumber >= 10:
            self.pushButtonAddClass.setDisabled(True)
        elif ClassNumber <= 1:
            self.pushButtonDeleteClass.setDisabled(True)

        diameter_type = self.model.getDiameterType(self.fuel)

        if diameter_type == 'rosin-rammler_law':
            self._updateRefusalButton()


    def _updateRefusalButton(self):
        """
        control refusal number between 1 and number of class for a define solid fuel
        """
        ClassNumber   = self.model.getClassNumber(self.fuel)
        RefusalNumber = self.model.getRefusalNumber(self.fuel)

        self.pushButtonDeleteRefusal.setEnabled(True)
        self.pushButtonAddRefusal.setEnabled(True)
        if RefusalNumber >= ClassNumber:
            self.pushButtonAddRefusal.setDisabled(True)
        elif RefusalNumber <= 1:
            self.pushButtonDeleteRefusal.setDisabled(True)


    def _updateOxidantButton(self):
        """
        control oxidant number between 1 and 3
        """
        OxidantNumber = self.model.getOxidantNumber()

        self.pushButtonAddOxidant.setEnabled(True)
        self.pushButtonDeleteOxidant.setEnabled(True)
        if OxidantNumber >= 3:
            self.pushButtonAddOxidant.setDisabled(True)
        elif OxidantNumber <= 1:
            self.pushButtonDeleteOxidant.setDisabled(True)


    def initializeDiameter(self):
        """
        initialize view with diameter type choice
        """
        self.modelClasses.deleteAll()

        key = self.model.getDiameterType(self.fuel)
        self.modelDiameter.setItem(str_model=key)

        ClassesNumber = self.model.getClassNumber(self.fuel)

        if key == 'automatic':
            self.treeViewRefusal.hide()
            self.pushButtonDeleteRefusal.hide()
            self.pushButtonAddRefusal.hide()

            for number in range(0, ClassesNumber):
                diam  = self.model.getDiameter(self.fuel, number+1)
                self.modelClasses.addItem(number + 1, diam)
        else:
            self.treeViewRefusal.show()
            self.modelRefusal.deleteAll()
            RefusalNumber = self.model.getRefusalNumber(self.fuel)

            for number in range(0, ClassesNumber):
                diam  = self.model.getMassPercent(self.fuel, number+1)
                self.modelClasses.addItem(number + 1, diam)

            for number in range(0, RefusalNumber):
                refusal = self.model.getRefusal(self.fuel, number+1)
                self.modelRefusal.addItem(number+1, refusal)
                log.debug("slotDeleteRefusal number + 1 = %i " % (number+1))
            self._updateRefusalButton()
            self.pushButtonDeleteRefusal.show()
            self.pushButtonAddRefusal.show()


    def initializeNOxView(self):
        """
        initialize NOx tabview for a define solid fuel
        """
        if self.model.getNOxFormationStatus() == 'on':
            self.checkBoxNOxFormation.setChecked(True)
            self.groupBoxNOxFormation.show()
            self.lineEditQPR.setText(QString(str(self.model.getNitrogenFraction(self.fuel))))
            self.lineEditNitrogenConcentration.setText \
                (QString(str(self.model.getNitrogenConcentration(self.fuel))))
            self.lineEditKobayashi1.setText(QString(str(self.model.getHCNParameter \
                (self.fuel, "HCN_NH3_partitionning_reaction_1"))))
            self.lineEditKobayashi2.setText(QString(str(self.model.getHCNParameter \
                (self.fuel, "HCN_NH3_partitionning_reaction_2"))))
        else:
            self.checkBoxNOxFormation.setChecked(False)
            self.groupBoxNOxFormation.hide()


    def initializeKineticsView(self):
        """
        initialize kinetic tabview for a define solid fuel
        """
        if self.model.getCO2KineticsStatus() == 'on':
            self.checkBoxCO2Kinetics.setChecked(True)
            self.groupBoxParametersCO2.show()
            self.lineEditConstCO2.setText(QString(str(self.model.getPreExponentialConstant(self.fuel, "CO2"))))
            self.lineEditEnergyCO2.setText(QString(str(self.model.getEnergyOfActivation(self.fuel, "CO2"))))

            key = self.model.getOrderOfReaction(self.fuel, "CO2")
            self.modelReactTypeCO2.setItem(str_model=key)
            if key =='1':
                self.labelUnitConstCO2.setText('kg/m<sup>2</sup>/s/atm')
            elif key =='0.5':
                self.labelUnitConstCO2.setText('kg/m<sup>2</sup>/s/atm<sup>1/2</sup>')
        else:
            self.checkBoxCO2Kinetics.setChecked(False)
            self.groupBoxParametersCO2.hide()

        if self.model.getH2OKineticsStatus() == 'on':
            self.checkBoxH2OKinetics.setChecked(True)
            self.groupBoxParametersH2O.show()
            self.lineEditConstH2O.setText(QString(str(self.model.getPreExponentialConstant(self.fuel, "H2O"))))
            self.lineEditEnergyH2O.setText(QString(str(self.model.getEnergyOfActivation(self.fuel, "H2O"))))

            key = self.model.getOrderOfReaction(self.fuel, "H2O")
            self.modelReactTypeH2O.setItem(str_model=key)
            if key =='1':
                self.labelUnitConstH2O.setText('kg/m<sup>2</sup>/s/atm')
            elif key =='0.5':
                self.labelUnitConstH2O.setText('kg/m<sup>2</sup>/s/atm<sup>1/2</sup>')
        else:
            self.checkBoxH2OKinetics.setChecked(False)
            self.groupBoxParametersH2O.hide()


    def initializeView(self):
        """
        initialize view for a define solid fuel
        """
        self.modelClasses = StandardItemModelClasses(self.model, self.fuel)
        self.treeViewClasses.setModel(self.modelClasses)
        self.treeViewClasses.resizeColumnToContents(0)
        self.treeViewClasses.resizeColumnToContents(1)

        self.initializeDiameter()
        self.initializeKineticsView()
        self.initializeNOxView()
        self._updateClassButton()

        # General (composition)
        self.lineEditC.setText(QString(str(self.model.getComposition(self.fuel, "C"))))
        self.lineEditH.setText(QString(str(self.model.getComposition(self.fuel, "H"))))
        self.lineEditO.setText(QString(str(self.model.getComposition(self.fuel, "O"))))
        self.lineEditN.setText(QString(str(self.model.getComposition(self.fuel, "N"))))
        self.lineEditS.setText(QString(str(self.model.getComposition(self.fuel, "S"))))
        self.lineEditPCI.setText(QString(str(self.model.getPCIValue(self.fuel))))
        self.lineEditCp.setText(QString(str(self.model.getProperty(self.fuel, "specific_heat_average"))))
        self.lineEditDensity.setText(QString(str(self.model.getProperty(self.fuel, "density"))))
        self.lineEditMoisture.setText(QString(str(self.model.getProperty(self.fuel, "moisture"))))
        self.lineEditVolatileMatter.setText(QString(str(self.model.getProperty(self.fuel, "volatile_matter"))))
        PCIChoice = self.model.getPCIChoice(self.fuel)
        self.modelPCI.setItem(str_model=PCIChoice)
        if PCIChoice == 'IGT_correlation':
            self.lineEditPCI.hide()
            self.comboBoxPCIType.hide()
            self.labelUnitPCI.hide()
        else:
            self.lineEditPCI.show()
            self.comboBoxPCIType.show()
            self.labelUnitPCI.show()
            PCIType = self.model.getPCIType(self.fuel)
            self.modelPCIType.setItem(str_model=PCIType)
            self.lineEditPCI.setText(QString(str(self.model.getPCIValue(self.fuel))))

        # Ashes
        self.lineEditAshesRatio.setText(QString(str(self.model.getProperty(self.fuel, "rate_of_ashes_on_mass"))))
        self.lineEditAshesEnthalpy.setText(QString(str(self.model.getProperty(self.fuel, "ashes_enthalpy"))))
        self.lineEditAshesCp.setText(QString(str(self.model.getProperty(self.fuel, "ashes_thermal_capacity"))))

        # Devolatilisation
        Y1Y2Choice = self.model.getY1Y2(self.fuel)
        self.modelY1Y2.setItem(str_model=Y1Y2Choice)
        if Y1Y2Choice == 'automatic_CHONS':
            self.frameY1Y2.hide()
        else:
            self.frameY1Y2.show()
        self.lineEditCoefY1.setText(QString(str(self.model.getY1StoichiometricCoefficient(self.fuel))))
        self.lineEditCoefY2.setText(QString(str(self.model.getY2StoichiometricCoefficient(self.fuel))))

        A1 = self.model.getDevolatilisationParameter(self.fuel, "A1_pre-exponential_factor")
        A2 = self.model.getDevolatilisationParameter(self.fuel, "A2_pre-exponential_factor")
        E1 = self.model.getDevolatilisationParameter(self.fuel, "E1_energy_of_activation")
        E2 = self.model.getDevolatilisationParameter(self.fuel, "E2_energy_of_activation")
        self.lineEditCoefA1.setText(QString(str(A1)))
        self.lineEditCoefA2.setText(QString(str(A2)))
        self.lineEditCoefE1.setText(QString(str(E1)))
        self.lineEditCoefE2.setText(QString(str(E2)))

        # Combustion heterogene
        self.lineEditConstO2.setText(QString(str(self.model.getPreExponentialConstant(self.fuel, "O2"))))
        self.lineEditEnergyO2.setText(QString(str(self.model.getEnergyOfActivation(self.fuel, "O2"))))

        key = self.model.getOrderOfReaction(self.fuel, "O2")
        self.modelReactTypeO2.setItem(str_model=key)
        if key =='1':
            self.labelUnitConstO2.setText('kg/m<sup>2</sup>/s/atm')
        elif key =='0.5':
            self.labelUnitConstO2.setText('kg/m<sup>2</sup>/s/atm<sup>1/2</sup>')

        key = self.model.getOxidantType()
        self.modelOxidantType.setItem(str_model=key)

        if self.model.getCoalCombustionModel() == 'homogeneous_fuel':
            moisture = self.model.getProperty(self.fuel, "moisture")
            self.lineEditMoisture.setText(QString(str(moisture)))
            self.labelMoisture.setDisabled(True)
            self.lineEditMoisture.setDisabled(True)


    @pyqtSignature("const QModelIndex &")
    def slotSelectCoal(self, text=None):
        """
        Display values for the current coal selected in the view.
        """
        row = self.treeViewCoals.currentIndex().row()
        log.debug("selectCoal row = %i "%row)

        self.fuel = row + 1

        self.initializeView()


    @pyqtSignature("")
    def slotCreateCoal(self):
        """ create a new coal"""
        # Init
        self.treeViewCoals.clearSelection()
        self.modelCoals.addItem()

        self.initializeView()

        # update Properties and scalars
        self.model.createCoalModelScalarsAndProperties()

        # update Buttons
        self._updateCoalButton()


    @pyqtSignature("")
    def slotDeleteCoal(self):
        """ cancel a coal"""
        row = self.treeViewCoals.currentIndex().row()
        log.debug("slotDeleteCoal row = %i "%row)

        if row == -1:
            return

        number = row + 1

        # update boundary conditions (1/2)
        # TODO : a deplacer dans le modele
        for zone in LocalizationModel('BoundaryZone', self.case).getZones():
            label = zone.getLabel()
            nature = zone.getNature()
            if nature == "inlet":
                bc = Boundary("coal_inlet", label, self.case)
                bc.deleteCoalFlow(number-1, self.model.getNumberCoal())

        self.model.deleteSolidFuel(number)

        # suppress item
        self.modelCoals.deleteItem(row)

        # First coal is selected
        row = 0
        self.fuel = row + 1

        self.initializeView()

        # Update buttons
        self._updateCoalButton()


    @pyqtSignature("")
    def slotCreateClass(self):
        """Create a new class"""
        self.model.createClass(self.fuel)

        # Init
        ClassNumber = self.model.getClassNumber(self.fuel)
        diameter_type = self.model.getDiameterType(self.fuel)
        if diameter_type == 'automatic':
            diam = self.model.getDiameter(self.fuel, ClassNumber)
        elif diameter_type == 'rosin-rammler_law':
            diam = self.model.getMassPercent(self.fuel, ClassNumber)
        self.modelClasses.addItem(ClassNumber, diam)

        log.debug("slotCreateClass number + 1 = %i " % (ClassNumber))

        self.model.createClassModelScalarsAndProperties(self.fuel)

        # FIXME: bug ici
        # TODO : a deplacer dans le modele
        # Update boundary conditions
        log.debug("slotCreateClass: number of classes: %i " % self.model.getClassNumber(self.fuel))
        for zone in LocalizationModel('BoundaryZone', self.case).getZones():
            if zone.getNature() == "inlet":
                b = Boundary("coal_inlet", zone.getLabel(), self.case)
                b.updateCoalRatios(self.fuel-1)

        # Update buttons
        self._updateClassButton()


    @pyqtSignature("")
    def slotDeleteClass(self):
        """ cancel a class diameter"""
        row = self.treeViewClasses.currentIndex().row()
        log.debug("slotDeleteClass  number = %i " % row)
        if row == -1:
            return

        number = row + 1

        self.model.deleteClass(self.fuel, number)

        # Init
        self.initializeDiameter()

        # Update buttons
        self._updateClassButton()


    @pyqtSignature("")
    def slotCreateRefusal(self):
        """Create a new refusal"""
        diameter = self.model.defaultValues()['diameter']

        self.model.createRefusal(self.fuel)

        # Init
        RefusalNumber = self.model.getRefusalNumber(self.fuel)
        refusal = self.model.getRefusal(self.fuel, RefusalNumber)
        self.modelRefusal.addItem(RefusalNumber, refusal)
        log.debug("slotCreateRefusal number + 1 = %i " % (RefusalNumber))

        # Update buttons
        self._updateRefusalButton()


    @pyqtSignature("")
    def slotDeleteRefusal(self):
        """ cancel a refusal"""
        row = self.treeViewRefusal.currentIndex().row()
        log.debug("slotDeleteRefusal  number = %i " % row)
        if row == -1:
            return

        number = row + 1

        self.model.deleteRefusal(self.fuel, number)

        # Init
        self.modelRefusal.deleteAll()
        RefusalNumber = self.model.getRefusalNumber(self.fuel)
        for number in range(0, RefusalNumber):
            refusal = self.model.getRefusal(self.fuel, number+1)
            self.modelRefusal.addItem(number+1, refusal)
            log.debug("slotDeleteRefusal number + 1 = %i " % (number+1))

        # Update buttons
        self._updateRefusalButton()


    @pyqtSignature("")
    def slotCreateOxidant(self):
        """Create a new oxidant"""
        self.model.createOxidant()
        num = self.model.getOxidantNumber()
        self.modelOxidants.addItem(str(num))

        log.debug("slotCreateOxidant number = %i " % num)

        # Update buttons
        self._updateOxidantButton()


    @pyqtSignature("")
    def slotDeleteOxidant(self):
        """ delete an oxidant"""
        row = self.tableViewOxidants.currentIndex().row()
        log.debug("slotDeleteOxidants number = %i " % row)
        if row == -1:
            return

        number = row + 1
        self.model.deleteOxidant(number)

        # TODO : a deplacer dans le modele
        # Update boundary conditions
        for zone in LocalizationModel('BoundaryZone', self.case).getZones():
            label = zone.getLabel()
            nature = zone.getNature()
            if nature == "inlet":
                bc = Boundary("coal_inlet", label, self.case)
                oxi_max = bc.getOxidantNumber()
                if oxi_max >= number:
                    bc.setOxidantNumber(oxi_max-1)

        self.modelOxidants.deleteAll()
        for number in range(0, self.model.getOxidantNumber()):
            self.modelOxidants.addItem(number+1)

        # Update buttons
        self._updateOxidantButton()


    @pyqtSignature("const QString&")
    def slotCComposition(self, text):
        """
        Change the C composition
        """
        composition, ok = text.toDouble()
        if self.sender().validator().state == QValidator.Acceptable:
            self.model.setComposition(self.fuel, "C", composition)
        else:
            msg = self.tr("This value must be between 0 and 100.")
            self.stbar.showMessage(msg, 2000)


    @pyqtSignature("const QString&")
    def slotHComposition(self, text):
        """
        Change the H composition
        """
        composition, ok = text.toDouble()
        if self.sender().validator().state == QValidator.Acceptable:
            self.model.setComposition(self.fuel, "H", composition)
        else:
            msg = self.tr("This value must be between 0 and 100.")
            self.stbar.showMessage(msg, 2000)


    @pyqtSignature("const QString&")
    def slotOComposition(self, text):
        """
        Change the O composition
        """
        composition, ok = text.toDouble()
        if self.sender().validator().state == QValidator.Acceptable:
            self.model.setComposition(self.fuel, "O", composition)
        else:
            msg = self.tr("This value must be between 0 and 100.")
            self.stbar.showMessage(msg, 2000)


    @pyqtSignature("const QString&")
    def slotNComposition(self, text):
        """
        Change the N composition
        """
        composition, ok = text.toDouble()
        if self.sender().validator().state == QValidator.Acceptable:
            self.model.setComposition(self.fuel, "N", composition)
        else:
            msg = self.tr("This value must be between 0 and 100.")
            self.stbar.showMessage(msg, 2000)


    @pyqtSignature("const QString&")
    def slotSComposition(self, text):
        """
        Change the S composition
        """
        composition, ok = text.toDouble()
        if self.sender().validator().state == QValidator.Acceptable:
            self.model.setComposition(self.fuel, "S", composition)
        else:
            msg = self.tr("This value must be between 0 and 100.")
            self.stbar.showMessage(msg, 2000)


    @pyqtSignature("const QString&")
    def slotPCI(self, text):
        """
        Change the PCI value
        """
        PCI, ok = text.toDouble()
        if self.sender().validator().state == QValidator.Acceptable:
            self.model.setPCIValue(self.fuel, PCI)


    @pyqtSignature("const QString&")
    def slotPCIType(self, text):
        """
        Change the PCI type
        """
        key = self.modelPCIType.dicoV2M[str(text)]
        self.model.setPCIType(self.fuel, key)


    @pyqtSignature("const QString&")
    def slotPCIChoice(self, text):
        """
        Change the PCI choice
        """
        key = self.modelPCI.dicoV2M[str(text)]
        self.model.setPCIChoice(self.fuel, key)
        if key == 'IGT_correlation':
            self.lineEditPCI.hide()
            self.comboBoxPCIType.hide()
            self.labelUnitPCI.hide()
        else:
            self.lineEditPCI.show()
            self.comboBoxPCIType.show()
            self.labelUnitPCI.show()
            PCIType = self.model.getPCIType(self.fuel)
            self.modelPCIType.setItem(str_model=PCIType)
            self.lineEditPCI.setText(QString(str(self.model.getPCIValue(self.fuel))))


    @pyqtSignature("const QString&")
    def slotDiameterType(self, text):
        """
        Change the diameter type
        """
        key = self.modelDiameter.dicoV2M[str(text)]
        self.model.setDiameterType(self.fuel, key)

        self.modelClasses = StandardItemModelClasses(self.model, self.fuel)
        self.treeViewClasses.setModel(self.modelClasses)
        self.treeViewClasses.resizeColumnToContents(0)
        self.treeViewClasses.resizeColumnToContents(1)

        self.initializeDiameter()
        self._updateClassButton()


    @pyqtSignature("const QString&")
    def slotVolatileMatter(self, text):
        """
        Change the volatile matter
        """
        value, ok = text.toDouble()
        if self.sender().validator().state == QValidator.Acceptable:
            self.model.setProperty(self.fuel, "volatile_matter", value)


    @pyqtSignature("const QString&")
    def slotThermalCapacity(self, text):
        """
        Change the thermal capacity
        """
        Cp, ok = text.toDouble()
        if self.sender().validator().state == QValidator.Acceptable:
            self.model.setProperty(self.fuel, "specific_heat_average", Cp)


    @pyqtSignature("const QString&")
    def slotDensity(self, text):
        """
        Change the density
        """
        density, ok = text.toDouble()
        if self.sender().validator().state == QValidator.Acceptable:
            self.model.setProperty(self.fuel, "density", density)


    @pyqtSignature("const QString&")
    def slotMoisture(self, text):
        """
        Change the moisture
        """
        moisture, ok = text.toDouble()
        if self.sender().validator().state == QValidator.Acceptable:
            self.model.setProperty(self.fuel, "moisture", moisture)
        else:
            msg = self.tr("This value must be between 0 and 100.")
            self.stbar.showMessage(msg, 2000)


    @pyqtSignature("const QString&")
    def slotAshesRatio(self, text):
        """
        Change the ashes ratio
        """
        ashesRatio, ok = text.toDouble()
        if self.sender().validator().state == QValidator.Acceptable:
            self.model.setProperty(self.fuel, "rate_of_ashes_on_mass", ashesRatio)
        else:
            msg = self.tr("This value must be between 0 and 100.")
            self.stbar.showMessage(msg, 2000)


    @pyqtSignature("const QString&")
    def slotAshesFormingEnthalpy(self, text):
        """
        Change the ashes forming enthalpy
        """
        ashesFormingEnthalpy, ok = text.toDouble()
        if self.sender().validator().state == QValidator.Acceptable:
            self.model.setProperty(self.fuel, "ashes_enthalpy", ashesFormingEnthalpy)


    @pyqtSignature("const QString&")
    def slotAshesThermalCapacity(self, text):
        """
        Change the ashes thermal capacity
        """
        ashesThermalCapacity, ok = text.toDouble()
        if self.sender().validator().state == QValidator.Acceptable:
            self.model.setProperty(self.fuel, "ashes_thermal_capacity", ashesThermalCapacity)


    @pyqtSignature("const QString&")
    def slotY1CH(self, text):
        """
        Change the Y1 stoichiometric coefficient
        """
        Y1CH, ok = text.toDouble()
        if self.sender().validator().state == QValidator.Acceptable:
            self.model.setY1StoichiometricCoefficient(self.fuel, Y1CH)


    @pyqtSignature("const QString&")
    def slotY2CH(self, text):
        """
        Change the Y2 stoichiometric coefficient
        """
        Y2CH, ok = text.toDouble()
        if self.sender().validator().state == QValidator.Acceptable:
            self.model.setY2StoichiometricCoefficient(self.fuel, Y2CH)


    @pyqtSignature("const QString&")
    def slotY1Y2(self, text):
        """
        Change the Y1Y2 type
        """
        key = self.modelY1Y2.dicoV2M[str(text)]
        self.model.setY1Y2(self.fuel, key)
        if key == 'automatic_CHONS':
            self.frameY1Y2.hide()
        else:
            self.frameY1Y2.show()
            self.lineEditCoefY1.setText(QString(str(self.model.getY1StoichiometricCoefficient(self.fuel))))
            self.lineEditCoefY2.setText(QString(str(self.model.getY2StoichiometricCoefficient(self.fuel))))


    @pyqtSignature("const QString&")
    def slotA1CH(self, text):
        """
        Change the pre exponential factor A1
        """
        A1CH, ok = text.toDouble()
        if self.sender().validator().state == QValidator.Acceptable:
            self.model.setDevolatilisationParameter(self.fuel, "A1_pre-exponential_factor", A1CH)


    @pyqtSignature("const QString&")
    def slotA2CH(self, text):
        """
        Change the pre exponentiel factor A2
        """
        A2CH, ok = text.toDouble()
        if self.sender().validator().state == QValidator.Acceptable:
            self.model.setDevolatilisationParameter(self.fuel, "A2_pre-exponential_factor", A2CH)


    @pyqtSignature("const QString&")
    def slotE1CH(self, text):
        """
        Change the energy of activation E1
        """
        E1CH, ok = text.toDouble()
        if self.sender().validator().state == QValidator.Acceptable:
            self.model.setDevolatilisationParameter(self.fuel, "E1_energy_of_activation", E1CH)


    @pyqtSignature("const QString&")
    def slotE2CH(self, text):
        """
        Change the Energy of activation E2
        """
        E2CH, ok = text.toDouble()
        if self.sender().validator().state == QValidator.Acceptable:
            self.model.setDevolatilisationParameter(self.fuel, "E2_energy_of_activation", E2CH)


    @pyqtSignature("const QString&")
    def slotPreExpoCstO2(self, text):
        """
        Change the pre exponential constant for O2
        """
        value, ok = text.toDouble()
        if self.sender().validator().state == QValidator.Acceptable:
            self.model.setPreExponentialConstant(self.fuel, "O2", value)


    @pyqtSignature("const QString&")
    def slotActivEnergyO2(self, text):
        """
        Change the energy of activation for O2
        """
        value, ok = text.toDouble()
        if self.sender().validator().state == QValidator.Acceptable:
            self.model.setEnergyOfActivation(self.fuel, "O2", value)


    @pyqtSignature("const QString&")
    def slotReactTypeO2(self, text):
        """
        Change the order of reaction of O2
        """
        key = self.modelReactTypeO2.dicoV2M[str(text)]
        self.model.setOrderOfReaction(self.fuel, "O2", key)
        if text =='1':
            self.labelUnitConstO2.setText('kg/m<sup>2</sup>/s/atm')
        elif text =='0.5':
            self.labelUnitConstO2.setText('kg/m<sup>2</sup>/s/atm<sup>1/2</sup>')


    @pyqtSignature("const QString&")
    def slotPreExpoCstCO2(self, text):
        """
        Change the preexponential constant for CO2
        """
        value, ok = text.toDouble()
        if self.sender().validator().state == QValidator.Acceptable:
            self.model.setPreExponentialConstant(self.fuel, "CO2", value)


    @pyqtSignature("const QString&")
    def slotActivEnergyCO2(self, text):
        """
        Change the energy of activation for CO2
        """
        value, ok = text.toDouble()
        if self.sender().validator().state == QValidator.Acceptable:
            self.model.setEnergyOfActivation(self.fuel, "CO2", value)


    @pyqtSignature("const QString&")
    def slotReactTypeCO2(self, text):
        """
        Change the order of reaction for CO2
        """
        key = self.modelReactTypeCO2.dicoV2M[str(text)]
        self.model.setOrderOfReaction(self.fuel, "CO2", key)
        if text =='1':
            self.labelUnitConstCO2.setText('kg/m<sup>2</sup>/s/atm')
        elif text =='0.5':
            self.labelUnitConstCO2.setText('kg/m<sup>2</sup>/s/atm<sup>1/2</sup>')


    @pyqtSignature("const QString&")
    def slotPreExpoCstH2O(self, text):
        """
        Change the pre exponential constant for H2O
        """
        value, ok = text.toDouble()
        if self.sender().validator().state == QValidator.Acceptable:
            self.model.setPreExponentialConstant(self.fuel, "H2O", value)


    @pyqtSignature("const QString&")
    def slotActivEnergyH2O(self, text):
        """
        Change the energy of activation for H2O
        """
        value, ok = text.toDouble()
        if self.sender().validator().state == QValidator.Acceptable:
            self.model.setEnergyOfActivation(self.fuel, "H2O", value)


    @pyqtSignature("const QString&")
    def slotReactTypeH2O(self, text):
        """
        Change the order of reaction
        """
        key = self.modelReactTypeH2O.dicoV2M[str(text)]
        self.model.setOrderOfReaction(self.fuel, "H2O", key)
        if text =='1':
            self.labelUnitConstH2O.setText('kg/m<sup>2</sup>/s/atm')
        elif text =='0.5':
            self.labelUnitConstH2O.setText('kg/m<sup>2</sup>/s/atm<sup>1/2</sup>')


    @pyqtSignature("const QString&")
    def slotQPR(self, text):
        """
        Change the nitrogen fraction
        """
        value, ok = text.toDouble()
        if self.sender().validator().state == QValidator.Acceptable:
            self.model.setNitrogenFraction(self.fuel, value)


    @pyqtSignature("const QString&")
    def slotNitrogenConcentration(self, text):
        """
        Change the nitrogen concentration
        """
        value, ok = text.toDouble()
        if self.sender().validator().state == QValidator.Acceptable:
            self.model.setNitrogenConcentration(self.fuel, value)


    @pyqtSignature("const QString&")
    def slotKobayashi1(self, text):
        """
        Change the nitrogen partition reaction of reaction 1
        """
        value, ok = text.toDouble()
        if self.sender().validator().state == QValidator.Acceptable:
            self.model.setHCNParameter(self.fuel, "HCN_NH3_partitionning_reaction_1", value)


    @pyqtSignature("const QString&")
    def slotKobayashi2(self, text):
        """
        Change the Nitrogen partition reaction of reaction 2
        """
        value, ok = text.toDouble()
        if self.sender().validator().state == QValidator.Acceptable:
            self.model.setHCNParameter(self.fuel, "HCN_NH3_partitionning_reaction_2", value)


    @pyqtSignature("const QString&")
    def slotOxidantType(self, text):
        """
        Change the oxidant type
        """
        key = self.modelOxidantType.dicoV2M[str(text)]
        self.model.setOxidantType(key)


    @pyqtSignature("bool")
    def slotNOxFormation(self, checked):
        """
        check box for NOx formation
        """
        status = 'off'
        if checked:
            status = 'on'
        self.model.setNOxFormationStatus(status)
        self.initializeNOxView()


    @pyqtSignature("bool")
    def slotCO2Kinetics(self, checked):
        """
        check box for CO2 kinetics
        """
        status = 'off'
        if checked:
            status = 'on'
        self.model.setCO2KineticsStatus(status)
        self.initializeKineticsView()


    @pyqtSignature("bool")
    def slotH2OKinetics(self, checked):
        """
        check box for H2O kinetics
        """
        status = 'off'
        if checked:
            status = 'on'
        self.model.setH2OKineticsStatus(status)
        self.initializeKineticsView()


    @pyqtSignature("int")
    def slotchanged(self, index):
        """
        Changed tab
        """
        self.case['current_tab'] = index


    def tr(self, text):
        """
        Translation
        """
        return text


#-------------------------------------------------------------------------------
# End
#-------------------------------------------------------------------------------
