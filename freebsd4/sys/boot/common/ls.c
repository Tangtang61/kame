/*
 * $FreeBSD: src/sys/boot/common/ls.c,v 1.8 1999/08/28 00:39:50 peter Exp $
 * From: $NetBSD: ls.c,v 1.3 1997/06/13 13:48:47 drochner Exp $
 */

/*
 * Copyright (c) 1993
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (c) 1996
 *	Matthias Drochner.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


#include <sys/param.h>
#include <ufs/ufs/dinode.h>
#include <ufs/ufs/dir.h>

#include <stand.h>
#include <string.h>

#include "bootstrap.h"

static char typestr[] = "?fc?d?b? ?l?s?w";

static int	ls_getdir(char **pathp);

COMMAND_SET(ls, "ls", "list files", command_ls);

static int
command_ls(int argc, char *argv[])
{
    int		fd;
    size_t	size;
    struct stat	sb;
    static char	dirbuf[DIRBLKSIZ];
    char	*buf, *path;
    char	lbuf[128];		/* one line */
    int		result, ch;
    int		verbose;
	
    result = CMD_OK;
    fd = -1;
    verbose = 0;
    optind = 1;
    optreset = 1;
    while ((ch = getopt(argc, argv, "l")) != -1) {
	switch(ch) {
	case 'l':
	    verbose = 1;
	    break;
	case '?':
	default:
	    /* getopt has already reported an error */
	    return(CMD_OK);
	}
    }
    argv += (optind - 1);
    argc -= (optind - 1);

    if (argc < 2) {
	path = "";
    } else {
	path = argv[1];
    }

    fd = ls_getdir(&path);
    if (fd == -1) {
	result = CMD_ERROR;
	goto out;
    }
    pager_open();
    pager_output(path);
    pager_output("\n");

    
    while ((size = read(fd, dirbuf, DIRBLKSIZ)) == DIRBLKSIZ) {
	struct direct  *dp, *edp;

	dp = (struct direct *) dirbuf;
	edp = (struct direct *) (dirbuf + size);

	while (dp < edp) {
	    if (dp->d_ino != (ino_t) 0) {

		if ((dp->d_namlen > MAXNAMLEN + 1) || (dp->d_type > sizeof(typestr))) {
		    /*
		     * This does not handle "old"
		     * filesystems properly. On little
		     * endian machines, we get a bogus
		     * type name if the namlen matches a
		     * valid type identifier. We could
		     * check if we read namlen "0" and
		     * handle this case specially, if
		     * there were a pressing need...
		     */
		    command_errmsg = "bad dir entry";
		    result = CMD_ERROR;
		    goto out;
		}
				
		if (strcmp(dp->d_name, ".") && strcmp(dp->d_name, "..")) {
		    if (verbose) {
			/* stat the file, if possible */
			sb.st_size = 0;
			buf = malloc(strlen(path) + strlen(dp->d_name) + 2);
			sprintf(buf, "%s/%s", path, dp->d_name);
			/* ignore return, could be symlink, etc. */
			if (stat(buf, &sb))
			    sb.st_size = 0;
			free(buf);
			sprintf(lbuf, " %c %8d %s\n", typestr[dp->d_type], (int)sb.st_size, dp->d_name);
		    } else
			sprintf(lbuf, " %c  %s\n", typestr[dp->d_type], dp->d_name);
		    if (pager_output(lbuf))
			goto out;
		}
	    }
	    dp = (struct direct *) ((char *) dp + dp->d_reclen);
	}
    }
 out:
    pager_close();
    if (fd != -1)
	close(fd);
    if (path != NULL)
	free(path);
    return(result);
}

/*
 * Given (path) containing a vaguely reasonable path specification, return an fd
 * on the directory, and an allocated copy of the path to the directory.
 */
static int
ls_getdir(char **pathp)
{
    struct stat	sb;
    int		fd;
    const char	*cp;
    char	*path, *tail;
    
    tail = NULL;
    fd = -1;

    /* one extra byte for a possible trailing slash required */
    path = malloc(strlen(*pathp) + 2);
    strcpy(path, *pathp);

    /* Make sure the path is respectable to begin with */
    if (archsw.arch_getdev(NULL, path, &cp)) {
	sprintf(command_errbuf, "bad path '%s'", path);
	goto out;
    }
    
    /* If there's no path on the device, assume '/' */
    if (*cp == 0)
	strcat(path, "/");

    fd = open(path, O_RDONLY);
    if (fd < 0) {
	sprintf(command_errbuf, "open '%s' failed: %s", path, strerror(errno));
	goto out;
    }
    if (fstat(fd, &sb) < 0) {
	sprintf(command_errbuf, "stat failed: %s", strerror(errno));
	goto out;
    }
    if (!S_ISDIR(sb.st_mode)) {
	sprintf(command_errbuf, "%s: %s", path, strerror(ENOTDIR));
	goto out;
    }

    *pathp = path;
    return(fd);

 out:
    free(path);
    *pathp = NULL;
    if (fd != -1)
	close(fd);
    return(-1);
}
