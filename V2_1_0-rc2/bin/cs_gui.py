#-------------------------------------------------------------------------------
#
#     This file is part of the Code_Saturne User Interface, element of the
#     Code_Saturne CFD tool.
#
#     Copyright (C) 1998-2011 EDF S.A., France
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
Parse command line arguments and launch the GUI.
"""

#-------------------------------------------------------------------------------
# Library modules import
#-------------------------------------------------------------------------------

import os, sys, string

from optparse import OptionParser

if not hasattr(sys, 'version_info') or sys.version_info <= (2, 4, 0, 'final'):
    raise SystemExit("Graphical user interface requires python 2.4 or later.")

#-------------------------------------------------------------------------------
# Third-party modules
#-------------------------------------------------------------------------------

try:
    from PyQt4.QtCore import *
    from PyQt4.QtGui  import *
except ImportError:
    print("\n  Error: Unable to import PyQt4.QtCore or PyQt4.QtGui modules.")
    print("  Please check your PyQt4 installation.\n")
    sys.exit(0)


if map(int, string.split(QT_VERSION_STR, ".")) < [4, 3, 0]:
    raise SystemExit("Graphical user interface requires Qt 4.3 or later "\
                     "(found %s)." % QT_VERSION_STR)


if map(int, string.split(PYQT_VERSION_STR, ".")) < [4, 3, 0]:
    raise SystemExit("Graphical user interface requires PyQt 4.3 or later "\
                     "(found %s)." % PYQT_VERSION_STR)

#-------------------------------------------------------------------------------
# Processes the passed command line arguments
#-------------------------------------------------------------------------------

def process_cmd_line(argv):
    """
    Processes the passed command line arguments.
    """

    parser = OptionParser(usage="usage: %prog [options]")

    parser.add_option("-f", "--file", dest="file_name", type="string",
                      metavar="<file>",
                      help="upload a previous case at the interface start")

    parser.add_option("-b", "--batch", dest="batch_file", type="string",
                      metavar="<batchfile>",
                      help="set batchrunning window with batch file")

    parser.add_option("-n", "--new", dest="new",
                      action="store_true",
                      help="open a new case")

    parser.add_option("-r", "--read-only", dest="read_only",
                      action="store_true",
                      help="load file in read only mode")

    parser.add_option("-z", "--no-splash", dest="splash_screen",
                      action="store_false",
                      help="load file in read only mode")

    parser.add_option("--no-tree", dest="tree_window",
                      action="store_false",
                      help="load file in read only mode")


    parser.set_defaults(read_only=False)
    parser.set_defaults(splash_screen=True)
    parser.set_defaults(tree_window=True)

    (options, args) = parser.parse_args(argv)


    if options.new and options.file_name:
        parser.error("Options --new and --file are mutually exclusive")

    if options.new:
        options.file_name = "new case"

    if options.batch_file and not options.file_name:
        parser.error("Option --batch requires --file")

    if len(args) > 0:
        if options.file_name or len(args) > 1:
            parser.error("Multiple filenames are given")
        else:
            options.file_name = args[0]

    batch_window = False
    if options.batch_file:
        batch_window = True
        options.batch_file = os.path.basename(options.batch_file)

    return options.file_name, options.splash_screen, \
        batch_window, options.batch_file, options.tree_window, options.read_only

#-------------------------------------------------------------------------------
# Main
#-------------------------------------------------------------------------------

def main(argv, pkg):
    """
    Start Qt and a session of the application.
    """

    # Test the package name to know which modules have to be imported
    if pkg.name == 'code_saturne':
        from Base.MainView import MainView
        icons_path = os.path.join(pkg.pkgpythondir, 'Base', 'icons')
    else:
        from core.MainView import MainView
        icons_path = os.path.join(pkg.pkgpythondir, 'core', 'icons')

    case, spl, batch_window, batch_file, tree_window, read_only \
       = process_cmd_line(argv)

    app = QApplication(argv)
    app.setOrganizationName("EDF S.A.")
    app.setOrganizationDomain(pkg.url)
    app.setApplicationName(pkg.name)
    #app.setWindowIcon(QIcon(":/icon.png"))
    app.connect(app, SIGNAL("lastWindowClosed()"), app, SLOT("quit()"))

    if spl:
        app.setOverrideCursor(QCursor(Qt.WaitCursor))
        pixmap = QPixmap('%s/splashscreen.png' % icons_path)
        splash = QSplashScreen(pixmap, Qt.WindowStaysOnTopHint)
        splash.setMask(pixmap.mask()) # this is usefull if the splashscreen is not a regular ractangle...
        splash.show()
        splash.showMessage("%(name)s %(vers)s starting..." \
                           % {'name': pkg.name, 'vers':pkg.version},
                           Qt.AlignHCenter | Qt.AlignVCenter, Qt.black)
        app.processEvents()
        QTimer.singleShot(1500, splash.hide)

    main = MainView(package = pkg,
                    cmd_case = case,
                    cmd_batch_window = batch_window,
                    cmd_batch_file = batch_file,
                    cmd_tree_window = tree_window,
                    cmd_read_only = read_only)

    try:
        main.show()
        if spl:
            app.processEvents()
            app.restoreOverrideCursor()
    except:
        print("\n  Unable to display a Qt window.")
        print("  Please check your display environment.\n")
        sys.exit(0)

    sys.exit(app.exec_())

#-------------------------------------------------------------------------------
# End
#-------------------------------------------------------------------------------
