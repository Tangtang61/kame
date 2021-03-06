/*
 * The new sysinstall program.
 *
 * This is probably the last attempt in the `sysinstall' line, the next
 * generation being slated to essentially a complete rewrite.
 *
 * $Id: ftp.c,v 1.18.2.14 1997/10/03 14:00:40 jkh Exp $
 *
 * Copyright (c) 1995
 *	Jordan Hubbard.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer,
 *    verbatim and that no modifications are made prior to this
 *    point in the file.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY JORDAN HUBBARD ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL JORDAN HUBBARD OR HIS PETS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, LIFE OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include "sysinstall.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <netdb.h>
#include <pwd.h>
#include <ftpio.h>

Boolean ftpInitted = FALSE;
static FILE *OpenConn;
int FtpPort;

/* Brings up attached network device, if any - takes FTP device as arg */
static Boolean
netUp(Device *dev)
{
    Device *netdev = (Device *)dev->private;

    if (netdev)
	return netdev->init(netdev);
    else
	return TRUE;	/* No net == happy net */
}

/* Brings down attached network device, if any - takes FTP device as arg */
static void
netDown(Device *dev)
{
    Device *netdev = (Device *)dev->private;

    if (netdev)
	netdev->shutdown(netdev);
}

Boolean
mediaInitFTP(Device *dev)
{
    int i, code;
    char *cp, *rel, *hostname, *dir;
    char *user, *login_name, password[80];

    if (ftpInitted)
	return TRUE;

    if (isDebug())
	msgDebug("Init routine for FTP called.\n");

    if (OpenConn) {
	fclose(OpenConn);
	OpenConn = NULL;
    }

    /* If we can't initialize the network, bag it! */
    if (!netUp(dev))
	return FALSE;

try:
    cp = variable_get(FetchKameKit ? VAR_KAME_FTP_PATH : VAR_FTP_PATH);
    if (!cp) {
	if (DITEM_STATUS(mediaSetFTP(NULL)) == DITEM_FAILURE || (cp = variable_get(FetchKameKit ? VAR_KAME_FTP_PATH : VAR_FTP_PATH)) == NULL) {
	    msgConfirm("Unable to get proper FTP path.  FTP media not initialized.");
	    netDown(dev);
	    return FALSE;
	}
    }

    hostname = variable_get(VAR_FTP_HOST);
    dir = variable_get(VAR_FTP_DIR);
    if (!hostname || !dir) {
	msgConfirm("Missing FTP host or directory specification.  FTP media not initialized,");
	netDown(dev);
	return FALSE;
    }
    user = variable_get(VAR_FTP_USER);
    login_name = (!user || !*user) ? "anonymous" : user;

    if (variable_get(VAR_FTP_PASS))
	SAFE_STRCPY(password, variable_get(VAR_FTP_PASS));
    else if (RunningAsInit)
	sprintf(password, "installer@%s", variable_get(VAR_HOSTNAME));
    else {
	struct passwd *pw;
	char *user;

	pw = getpwuid(getuid());
	user = pw ? pw->pw_name : "ftp";
	sprintf(password, "%s@%s", user, variable_get(VAR_HOSTNAME));
    }
    msgNotify("Logging in to %s@%s..", login_name, hostname);
    if ((OpenConn = ftpLogin(hostname, login_name, password, FtpPort, isDebug(), &code)) == NULL) {
	msgConfirm("Couldn't open FTP connection to %s:\n  %s.", hostname, ftpErrString(code));
	goto punt;
    }

    ftpPassive(OpenConn, !strcmp(variable_get(VAR_FTP_STATE), "passive"));
    ftpBinary(OpenConn);
    if (dir && *dir != '\0') {
	if ((i = ftpChdir(OpenConn, dir)) != 0) {
	    if (i == 550)
		msgConfirm("No such directory ftp://%s/%s\n"
			   "please check your URL and try again.", hostname, dir);
	    else
		msgConfirm("FTP chdir to ftp://%s/%s returned error status:\n  %s.", hostname, dir, ftpErrString(i));
	    goto punt;
	}
    }

    /* Give it a shot - can't hurt to try and zoom in if we can, unless the release is set to
       __RELEASE or "none" which signifies that it's not set */
    rel = variable_get(VAR_RELNAME);
    if (strcmp(rel, "__RELEASE") && strcmp(rel, "none") && !FetchKameKit)
	i = ftpChdir(OpenConn, rel);
    else
	i = 0;
    if (i) {
	if (!msgYesNo("Warning:  Can't CD to `%s' distribution on this\n"
		      "FTP server.  You may need to visit a different server for\n"
		      "the release you're trying to fetch or go to the Options\n"
		      "menu and to set the release name to explicitly match what's\n"
		      "available on %s (or set to \"none\").\n\n"
		      "Would you like to select another FTP server?",
		      rel, hostname)) {
	    variable_unset(FetchKameKit ? VAR_KAME_FTP_PATH : VAR_FTP_PATH);
	    if (DITEM_STATUS(mediaSetFTP(NULL)) == DITEM_FAILURE)
		goto punt;
	    else
		goto try;
	}
	else
	    goto punt;
    }
    if (isDebug())
	msgDebug("mediaInitFTP was successful (logged in and chdir'd)\n");
    ftpInitted = TRUE;
    return TRUE;

punt:
    ftpInitted = FALSE;
    if (OpenConn != NULL) {
	fclose(OpenConn);
	OpenConn = NULL;
    }
    netDown(dev);
    variable_unset(FetchKameKit ? VAR_KAME_FTP_PATH : VAR_FTP_PATH);
    return FALSE;
}

FILE *
mediaGetFTP(Device *dev, char *file, Boolean probe)
{
    int nretries = 1;
    FILE *fp;
    char *try, buf[PATH_MAX];

    if (!OpenConn) {
	msgDebug("No FTP connection open, can't get file %s\n", file);
	return NULL;
    }

    try = file;
    while ((fp = ftpGet(OpenConn, try, 0)) == NULL) {
	int ftperr = ftpErrno(OpenConn);

	/* If a hard fail, try to "bounce" the ftp server to clear it */
	if (ftperr != 550) {
	    if (ftperr != 421)	/* Timeout? */
		variable_unset(FetchKameKit ? VAR_KAME_FTP_PATH : VAR_FTP_PATH);
	    /* If we can't re-initialize, just forget it */
	    dev->shutdown(dev);
	    if (!dev->init(dev)) {
		netDown(dev);
		if (OpenConn) {
		    fclose(OpenConn);
		    OpenConn = NULL;
		}
		variable_unset(FetchKameKit ? VAR_KAME_FTP_PATH : VAR_FTP_PATH);
		return NULL;
	    }
	}
	else if (probe)
	    return NULL;
	else if (FetchKameKit) {
	    /* Try some alternatives */
	    switch (nretries++) {
	    case 1:
		sprintf(buf, "kame/%s", file);
		try = buf;
		break;

	    case 2:
		try = file;
		break;
	    }
	}
	else {
	    /* Try some alternatives */
	    switch (nretries++) {
	    case 1:
		sprintf(buf, "dists/%s", file);
		try = buf;
		break;

	    case 2:
		sprintf(buf, "%s/%s", variable_get(VAR_RELNAME), file);
		try = buf;
		break;

	    case 3:
		sprintf(buf, "%s/dists/%s", variable_get(VAR_RELNAME), file);
		try = buf;
		break;

	    case 4:
		try = file;
		break;
	    }
	}
    }
    return fp;
}

void
mediaShutdownFTP(Device *dev)
{
    if (!ftpInitted)
	return;

    msgDebug("FTP shutdown called.  OpenConn = %x\n", OpenConn);
    if (OpenConn != NULL) {
	fclose(OpenConn);
	OpenConn = NULL;
    }
    ftpInitted = FALSE;
}
