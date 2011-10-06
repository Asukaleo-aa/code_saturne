# -*- coding: utf-8 -*-

#-------------------------------------------------------------------------------

# This file is part of Code_Saturne, a general-purpose CFD tool.
#
# Copyright (C) 1998-2011 EDF S.A.
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
This module defines basic classes used for the Pages construction.

This module defines the following classes:
- ComboModel
- IntValidator
- DoubleValidator
- RegExpValidator
"""

#-------------------------------------------------------------------------------
# Library modules import
#-------------------------------------------------------------------------------

import sys
import logging

#-------------------------------------------------------------------------------
# Third-party modules
#-------------------------------------------------------------------------------

from PyQt4 import QtGui, QtCore

#-------------------------------------------------------------------------------
# Application modules import
#-------------------------------------------------------------------------------

from Base.Toolbox import GuiParam
from Base.Common import LABEL_LENGTH_MAX

#-------------------------------------------------------------------------------
# log config
#-------------------------------------------------------------------------------

logging.basicConfig()
log = logging.getLogger("QtPage")
log.setLevel(GuiParam.DEBUG)

#-------------------------------------------------------------------------------
# QComboBox model
#-------------------------------------------------------------------------------

class ComboModel:
    """
    Class to build a model (QStandardItemModel) used with a QComboBox.

    Main attributes of class are:

    combo: QComboBox passed as arguments. It uses the model
    model: QStandardItemModel which contains items

    dicoV2M: correspondance between strings in Qt view and strings in parameters
    dicoM2V: correspondance between strings in parameters and strings in Qt view

    items: tuple which contains all model strings (usefull to get its index in the model)
    """
    def __init__(self, combo, rows=0, columns=0):
        """
        Initialization
        """
        self.combo   = combo

        self.rows    = rows
        self.columns = columns
        self.last    = 0

        self.model   = QtGui.QStandardItemModel()
        self.model.clear()
        self.model.setRowCount(rows)
        self.model.setColumnCount(columns)

        self.dicoV2M = {}
        self.dicoM2V = {}

        self.items   = []
        self.combo.setModel(self.model)


    def addItem(self, str_view, str_model=""):
        """
        Insert an item in the model.

        str_view: string to be displayed in the view.
        For example, 'Eulerian/Lagrangian Multi-phase Treatment'

        str_model: correponding string used in the model.
        For example, 'lagrangian'
        """
        item  = QtGui.QStandardItem(QtCore.QString(str(str_view)))
        index = self.last
        self.model.setItem(index, item)

        self.last = index + 1

        if not str_model: str_model = str_view

        self.items.append(str_model)

        self.dicoM2V[str_model] = str_view
        self.dicoV2M[str_view]  = str_model


    def modifyItem(self, old_str_view, new_str_view, new_str_model=""):
        """
        Modify string names.
        """
        if old_str_view in self.items:
            index = self.items.index(old_str_view)
            self.items[index] = new_str_view

            old_str_model = dicoV2M[old_str_view]
            if new_str_model == "":
                new_str_model = old_str_model

            del self.dicoM2V[str_model]
            del self.dicoV2M[str_view]

            self.dicoM2V[new_str_model] = new_str_view
            self.dicoV2M[new_str_view]  = new_str_model


    def delItem(self, index=None, str_model="", str_view=""):
        """
        Remove the item specified with its index or a string.
        """
        if index is not None:
            self.__deleteItem(index)

        elif str_model:
            index = self.items.index(str_model)
            self.__deleteItem(index)

        elif str_view:
            str_model = self.dicoV2M[str_view]
            index = self.items.index(str_model)
            self.__deleteItem(index)


    def __deleteItem(self, index):
        """
        Delete the item specified with its index
        """
        str_view  = self.items[index]
        str_model = self.dicoV2M[str_view]
        del self.items[index]
        del self.dicoV2M[str_view]
        del self.dicoM2V[str_model]


    def __disableItem(self, index):
        """
        Disable the item specified with its index
        """
        self.model.item(index).setEnabled(False)


    def __enableItem(self, index):
        """
        Enable the item specified with its index
        """
        self.model.item(index).setEnabled(True)


    def disableItem(self, index=None, str_model="", str_view=""):
        """
        Disable the item specified with its index or a string.
        """
        if index is not None:
            self.__disableItem(index)

        elif str_model:
            index = self.items.index(str_model)
            self.__disableItem(index)

        elif str_view:
            str_model = self.dicoV2M[str_view]
            index = self.items.index(str_model)
            self.__disableItem(index)


    def enableItem(self, index=None, str_model="", str_view=""):
        """
        Enable the item specified with its index or a string.
        """
        if index is not None:
            self.__enableItem(index)

        elif str_model:
            index = self.items.index(str_model)
            self.__enableItem(index)

        elif str_view:
            str_model = self.dicoV2M[str_view]
            index = self.items.index(str_model)
            self.__enableItem(index)


    def setItem(self, index=None, str_model="", str_view=""):
        """
        Set item as current.
        """
        if index is not None:
            self.combo.setCurrentIndex(index)

        elif str_model:
            index = self.items.index(str_model)
            self.combo.setCurrentIndex(index)

        elif str_view:
            str_model = self.dicoV2M[str_view]
            index = self.items.index(str_model)
            self.combo.setCurrentIndex(index)


    def getIndex(self, str_model="", str_view=""):
        """
        Get the index for a string.
        """
        if str_model:
            index = self.items.index(str_model)

        elif str_view:
            str_model = self.dicoV2M[str_view]
            index = self.items.index(str_model)

        return index


    def getItems(self):
        """
        Get the tuple of items.
        """
        return self.items

#-------------------------------------------------------------------------------
# Validators for editors
#-------------------------------------------------------------------------------

vmax = sys.maxint
vmax = 2147483647
vmin = -vmax

class IntValidator(QtGui.QIntValidator):
    """
    Validator for integer data.
    """
    def __init__(self, parent, min=vmin, max=vmax):
        """
        Initialization for validator
        """
        QtGui.QIntValidator.__init__(self, parent)
        self.parent = parent
        self.state = QtGui.QValidator.Invalid
        self.__min = min
        self.__max = max

        if type(min) != int or type(max) != int:
            raise ValueError("The given parameters are not integers (warning: long are not allowed).")
        self.setBottom(min)
        self.setTop(max)

        self.exclusiveMin = False
        self.exclusiveMax = False
        self.exclusiveValues = []

        self.default = 0
        self.fix = False

        msg = ""
        if min > vmin and max == vmax:
            msg = self.tr("The integer value must be greater than or equal to %i" % min)
        elif min == vmin and max < vmax:
            msg = self.tr("The integer value must be lower than or equal to %i" % max)
        elif min > vmin and max < vmax:
            msg = self.tr("The integer value must be between %i and %i" % (min, max))

        self.parent.setStatusTip(QtCore.QString(msg))


    def setExclusiveMin(self, b=True):
        if type(b) != bool:
            raise ValueError("The given parameter is not a boolean.")
        self.exclusiveMin = b

        msg = ""
        if self.__min > vmin and self.__max == vmax:
            msg = self.tr("The integer value must be greater than %i" % self.__min)
        elif self.__min == vmin and self.__max < vmax:
            msg = self.tr("The integer value must be lower than or equal to %i" % self.__max)
        elif self.__min > vmin and self.__max < vmax:
            msg = self.tr("The integer value must be greater %i and lower than or equal to %i" % (self.__min, self.__max))

        self.parent.setStatusTip(QtCore.QString(msg))


    def setExclusiveMax(self, b=True):
        if type(b) != bool:
            raise ValueError("The given parameter is not a boolean.")
        self.exclusiveMax = b

        msg = ""
        if self.__min > vmin and self.__max == vmax:
            msg = self.tr("The integer value must be greater than or equal to %i" % self.__min)
        elif self.__min == vmin and self.__max < vmax:
            msg = self.tr("The integer value must be lower than %i" % self.__max)
        elif self.__min > vmin and self.__max < vmax:
            msg = self.tr("The integer value must be greater than or equal to %i and lower than %i" % (self.__min, self.__max))

        self.parent.setStatusTip(QtCore.QString(msg))


    def setExclusiveValues(self, l):
        if type(l) != list and type(l) != tuple:
            raise ValueError("The given parameter is not a list or a tuple.")
        self.exclusiveValues = l

        msg = ""
        for v in l:
            if self.__min > vmin or self.__max < vmax:
                msg = self.tr("All integers value must be greater than %i and lower than %i" % (self.__min, self.__max))

        self.parent.setStatusTip(QtCore.QString(msg))


    def setFixup(self, v):
        if type(v) != int:
            raise ValueError("The given parameter is not an integer.")
        self.default = v
        self.fix = True


    def fixup(self, string):
        if self.fix:
            if string.length() == 0:
                string.truncate(0)
                string += str(self.default)


    def validate(self, string, pos):
        """
        Validation method.

        QValidator.Invalid       0  The string is clearly invalid.
        QValidator.Intermediate  1  The string is a plausible intermediate value during editing.
        QValidator.Acceptable    2  The string is acceptable as a final result; i.e. it is valid.
        """
        state = QtGui.QIntValidator.validate(self, string, pos)[0]

        x, valid = string.toInt()

        if state == QtGui.QValidator.Acceptable:
            if self.exclusiveMin and x == self.bottom():
                state = QtGui.QValidator.Intermediate
            elif self.exclusiveMax and x == self.top():
                state = QtGui.QValidator.Intermediate
            elif x in self.exclusiveValues:
                state = QtGui.QValidator.Intermediate

        palette = self.parent.palette()

        if not valid or state == QtGui.QValidator.Intermediate:
            palette.setColor(QtGui.QPalette.Text, QtGui.QColor("red"))
            self.parent.setPalette(palette)
        else:
            palette.setColor(QtGui.QPalette.Text, QtGui.QColor("black"))
            self.parent.setPalette(palette)

        self.state = state

        return (state, pos)


    def tr(self, text):
        """
        """
        return text


class DoubleValidator(QtGui.QDoubleValidator):
    """
    Validator for real data.
    """
    def __init__(self, parent, min=-1.e99, max=1.e99):
        """
        Initialization for validator
        """
        QtGui.QDoubleValidator.__init__(self, parent)
        self.parent = parent
        self.state = QtGui.QValidator.Invalid
        self.__min = min
        self.__max = max

        self.setNotation(self.ScientificNotation)

        if type(min) != float or type(max) != float:
            raise ValueError("The given parameters are not floats.")
        self.setBottom(min)
        self.setTop(max)

        self.exclusiveMin = False
        self.exclusiveMax = False

        self.default = 0.0
        self.fix = False

        msg = ""
        if min > -1.e99 and max == 1.e99:
            msg = self.tr("The float value must be greater than %.1f" % min)
        elif min == -1.e99 and max < 1.e99:
            msg = self.tr("The float value must be lower than %.1f" % max)
        elif min > -1.e99 and max < 1.e99:
            msg = self.tr("The float value must be between than %.1f and %.1f" % (min, max))

        self.parent.setStatusTip(QtCore.QString(msg))


    def setExclusiveMin(self, b=True):
        if type(b) != bool:
            raise ValueError("The given parameter is not a boolean.")
        self.exclusiveMin = b

        msg = ""
        if self.__min > -1.e99 and self.__max == 1.e99:
            msg = self.tr("The float value must be greater than %.1f" % self.__min)
        elif self.__min == -1.e99 and self.__max < 1.e99:
            msg = self.tr("The float value must be lower than or equal to %.1f" % self.__max)
        elif self.__min > -1.e99 and self.__max < 1.e99:
            msg = self.tr("The float value must be greater than %.1f and lower than or equal to %.1f" % (self.__min, self.__max))

        self.parent.setStatusTip(QtCore.QString(msg))


    def setExclusiveMax(self, b=True):
        if type(b) != bool:
            raise ValueError("The given parameter is not a boolean.")
        self.exclusiveMax = b

        msg = ""
        if self.__min > -1.e99 and self.__max == 1.e99:
            msg = self.tr("The float value must be greater than or equal to %.1f" % self.__min)
        elif self.__min == -1.e99 and self.__max < 1.e99:
            msg = self.tr("The float value must be lower than %.1f" % self.__max)
        elif self.__min > -1.e99 and self.__max < 1.e99:
            msg = self.tr("The float value must be greater than or equal to %.1f and lower than %.1f" % (self.__min, self.__max))

        self.parent.setStatusTip(QtCore.QString(msg))


    def setFixup(self, v):
        if type(v) != float:
            raise ValueError("The given parameter is not a float.")
        self.default = v
        self.fix = True


    def fixup(self, string):
        if self.fix:
            if string.length() == 0:
                string.truncate(0)
                string += str(self.default)


    def validate(self, string, pos):
        """
        Validation method.

        QValidator.Invalid       0  The string is clearly invalid.
        QValidator.Intermediate  1  The string is a plausible intermediate value during editing.
        QValidator.Acceptable    2  The string is acceptable as a final result; i.e. it is valid.
        """
        state = QtGui.QDoubleValidator.validate(self, string, pos)[0]

        x, valid = string.toDouble()

        if state == QtGui.QValidator.Acceptable:
            if self.exclusiveMin and x == self.bottom():
                state = QtGui.QValidator.Intermediate
            elif self.exclusiveMax and x == self.top():
                state = QtGui.QValidator.Intermediate

        palette = self.parent.palette()

        if not valid or state == QtGui.QValidator.Intermediate:
            palette.setColor(QtGui.QPalette.Text, QtGui.QColor("red"))
            self.parent.setPalette(palette)
        else:
            palette.setColor(QtGui.QPalette.Text, QtGui.QColor("black"))
            self.parent.setPalette(palette)

        self.state = state

        return (state, pos)


    def tr(self, text):
        """
        """
        return text


class RegExpValidator(QtGui.QRegExpValidator):
    """
    Validator for regular expression.
    """
    def __init__(self, parent, rx):
        """
        Initialization for validator
        """
        QtGui.QRegExpValidator.__init__(self, parent)
        self.parent = parent
        self.state = QtGui.QRegExpValidator.Invalid

        self.__validator = QtGui.QRegExpValidator(rx, parent)

        if "{1," + str(LABEL_LENGTH_MAX) + "}" in rx.pattern():
            msg = self.tr("The maximum length of the label is %i characters" % LABEL_LENGTH_MAX)
            self.parent.setStatusTip(QtCore.QString(msg))


    def validate(self, string, pos):
        """
        Validation method.

        QValidator.Invalid       0  The string is clearly invalid.
        QValidator.Intermediate  1  The string is a plausible intermediate value during editing.
        QValidator.Acceptable    2  The string is acceptable as a final result; i.e. it is valid.
        """
        state = self.__validator.validate(string, pos)[0]

        palette = self.parent.palette()

        if state == QtGui.QValidator.Intermediate:
            palette.setColor(QtGui.QPalette.Text, QtGui.QColor("red"))
            self.parent.setPalette(palette)
        else:
            palette.setColor(QtGui.QPalette.Text, QtGui.QColor("black"))
            self.parent.setPalette(palette)

        self.state = state

        return (state, pos)


    def tr(self, text):
        """
        """
        return text

#-------------------------------------------------------------------------------
# Paint in green a given widget
#-------------------------------------------------------------------------------

def setGreenColor(w, green=True):
    """
    Paint in green color the QWidget I{w} if I{green} is equal to C{True}.
    If not, the QWidget I{w} is paint with the color of its parents.

    @type w: C{QWidget}
    @param w: widget to paint
    @type green: C{True} or C{False}
    @param green: I{w} is paint in green if C{True}
    """
    if green:
        color = QtGui.QColor(QtCore.Qt.green)
    else:
        color = w.parentWidget().palette().color(QtGui.QPalette.Window)

    w.setPalette(QtGui.QPalette(color))

#-------------------------------------------------------------------------------
# End of QtPage
#-------------------------------------------------------------------------------
