#!/usr/bin/env python
# -*- coding: utf-8 -*-
#-------------------------------------------------------------------------------
#
#     This file is part of the Code_Saturne Scripts, element of the
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

#-------------------------------------------------------------------------------
# Standard modules import
#-------------------------------------------------------------------------------

import os, sys
import string
import pwd
import platform

from optparse import OptionParser
from datetime import datetime, date

import smtplib
from email.MIMEMultipart import MIMEMultipart
from email.MIMEBase import MIMEBase
from email.MIMEText import MIMEText
from email.Utils import COMMASPACE, formatdate
from email import Encoders

#-------------------------------------------------------------------------------
# Application modules import
#-------------------------------------------------------------------------------

from autovnv.Study import Studies

#-------------------------------------------------------------------------------
# Processes the passed command line arguments
#-------------------------------------------------------------------------------

def process_cmd_line(argv, pkg):
    """
    Processes the passed command line arguments.
    """
    parser = OptionParser(usage="usage: %prog [options]")

    parser.add_option("-f", "--file", dest="filename", type="string",
                      metavar="FILE", help="xml FILE of parameters")

    parser.add_option("-q", "--quiet",
                      action="store_true", dest="verbose", default=False,
                      help="don't print status messages to stdout")

    parser.add_option("-r", "--run",
                      action="store_true", dest="runcase", default=False,
                      help="run all cases")

    parser.add_option("-c", "--compare",
                      action="store_true", dest="compare", default=False,
                      help="compare results between repository and destination")

    parser.add_option("-p", "--post",
                      action="store_true", dest="post", default=False,
                      help="postprocess results of computations")

    parser.add_option("-m", "--mail", dest="addresses", default="",
                      type="string", metavar="ADDRESS1 ADDRESS2 ...",
                      help="addresses for sending the reports")

    (options, args) = parser.parse_args(argv)

    return  options.filename, options.verbose, options.runcase, \
        options.compare, options.post, options.addresses

#-------------------------------------------------------------------------------
# Send the report.
#-------------------------------------------------------------------------------

def sendmail(code_name, report, to, files):
    """
    Build the mail to be send.
    """
    # Note that the TO variable must be a list, and that you have
    # to add the From, To, and Subject headers to the message yourself.
    # The TO argument to the sendmail method is only used to route the
    # message to the right recipients, and doesn't have to match the headers
    # in the message itself. This can be used to implement Bcc headers; to send
    # a blind copy, add the address to the TO argument, but do not include
    # it in the To header.

    SERVER  = "localhost"
    FROM    = "saturne-support@edf.fr"
    TO      = to
    SUBJECT = "%s. Auto V&V %s: " % code_name, date.today()
    TEXT    = report
    RETOUR  = "An error occurs during the sending of the log mail"
    FILES   = files
    MESSAGE = """From: %s\nTo: %s\nSubject: %s\n\n%s""" % (FROM, ", ".join(TO), SUBJECT, TEXT)

    try:
        send_mail(FROM, TO, SUBJECT, MESSAGE, FILES, SERVER)
    except:
        send_mail(FROM, TO, "[ERROR] %s" % SUBJECT, retour, [], SERVER)


def send_mail(send_from, send_to, subject, text, files=[], server="localhost"):
    """
    Send the report by mail.
    """
    assert type(send_to) == list
    assert type(files)   == list

    msg = MIMEMultipart()
    msg['From']    = send_from
    msg['To']      = COMMASPACE.join(send_to)
    msg['Date']    = formatdate(localtime=True)
    msg['Subject'] = subject
    msg.attach( MIMEText(text) )

    for f in files:
        part = MIMEBase('application', "octet-stream")
        part.set_payload( open(f,"rb").read())
        Encoders.encode_base64(part)
        part.add_header('Content-Disposition',
                        'attachment; filename="%s"' % os.path.basename(f))
        msg.attach(part)

    smtp = smtplib.SMTP(server)
    smtp.sendmail(code_name, send_from, send_to, msg.as_string())
    smtp.close()

#-------------------------------------------------------------------------------
# Helper to find the release of Linux
#-------------------------------------------------------------------------------

def release():
    p = "/etc/issue"
    if os.path.isfile(p):
        f = open(p, 'r')
        issue = f.read()[:-1]
        f.close()
    else:
        issue = ""
    return issue

#-------------------------------------------------------------------------------
# Start point of Auto V & V script
#-------------------------------------------------------------------------------

def runAutoverif(pkg, opt_f, opt_v, opt_r, opt_c, opt_p, opt_to):
    """
    Main function
      1. parse the command line,
      2. read the file of parameters
      3. create all studies,
      4. compile sources
      5. run all cases
      6. compare results
      7. plot result
      8. reporting by mail
    """

    # Scripts

    exe = os.path.join(pkg.bindir, pkg.name)
    dif = pkg.get_io_dump()

    for p in exe, dif:
        if not os.path.isfile(p):
            print "Error: executable %s not found." % p
            sys.exit(1)

    dif += " -d"

    # Read the file of parameters

    studies = Studies(opt_f, opt_v, opt_r, opt_c, opt_p, exe, dif)
    os.chdir(studies.getDestination())

    # Print header

    studies.reporting(" ----------")
    studies.reporting(" Auto V & V")
    studies.reporting(" ----------\n")
    studies.reporting(" Code name:         " + pkg.name)
    studies.reporting(" Kernel version:    " + pkg.version)
    studies.reporting(" Install directory: " + pkg.exec_prefix)
    studies.reporting(" File dump:         " + dif)
    studies.reporting(" Repository:        " + studies.getRepository())
    studies.reporting(" Destination:       " + studies.getDestination())
    studies.reporting("\n Informations:")
    studies.reporting(" -------------\n")
    studies.reporting(" Date:               " + datetime.now().strftime("%A %B %d %H:%M:%S %Y"))
    studies.reporting(" Platform:           " + platform.platform())
    studies.reporting(" Computer:           " + platform.uname()[1] + "  " + release())
    studies.reporting(" Process Id:         " + str(os.getpid()))
    studies.reporting(" User name:          " + pwd.getpwuid(os.getuid())[0])
    studies.reporting(" Working directory:  " + os.getcwd())
    studies.reporting("\n")

    # Create all studies and all cases

    studies.createStudies()

    # Compile sources of all cases

    studies.compilation()

    # Update and run all cases

    if opt_r:
        studies.run()

    if opt_c:
        studies.check_compare()
        studies.compare()

    # Postprocess results

    if opt_p:
        studies.check_script()
        studies.scripts()
        studies.check_plot()
        studies.plot()

    studies.reporting("\n -----------------")
    studies.reporting(" End of Auto V & V")
    studies.reporting(" -----------------")

    # Reporting
    # TODO : *) inserer les images vtk dans le pdf
    #        *) temps de calcul

    attached_file = studies.build_reports("report_global", "report_detailed")
    if opt_to:
        sendmail(pkg.code_name, studies.logs(), to, attached_file)

#-------------------------------------------------------------------------------
# Main
#-------------------------------------------------------------------------------

def main(argv, pkg):
    """
    Main function.
    """

    # Command line

    opt_f, opt_v, opt_c, opt_r, opt_p, addresses = process_cmd_line(argv, pkg)
    opt_to  = string.split(addresses)

    retcode = runAutoverif(pkg, opt_f, opt_v, opt_c, opt_r, opt_p, opt_to)

    sys.exit(retcode)

#-------------------------------------------------------------------------------
