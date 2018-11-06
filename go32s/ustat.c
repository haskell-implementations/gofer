#include <string.h>
#include <errno.h>
#include <dir.h>
#include <sys/stat.h>


/*
 * An improved version of stat(). This version ensures that "." and ".."
 * appear to exist in all directories, including Novell network drives, and
 * that the root directory also returns the correct result.
 *
 * This function replaces any \'s with /'s in the input string, it should
 * really take a copy of the string first, but this isn't necessary for go32
 * which uses a transfer buffer between the program and the DOS extender
 * anyway.
 *
 * Chris Boucher ccb@southampton.ac.uk
 */


/*
 * I found the following function on SIMTEL20, it dates back to 1987 but
 * there's no mention of who the author was. I've modified/fixed it quite
 * a bit. It neatly avoids any problems with "." and ".." on Novell networks.
 */
static int rootpath(const char *relpath, char *fullpath);


int unixlike_stat(char *name, struct stat *buf) {
	static char path[2 * MAXPATH];
	char *s = name;
	int len;

	/* First off, try the standard stat(), if that works then all is well. */
	if (stat(name, buf) == 0) {
		return 0;
	}

	/* Swap all \'s for /'s. */
	while (*s) {
		if (*s == '\\') *s = '/';
		s++;
	}

	/* Convert path name into root based cannonical form. */
	if (rootpath(name, path) != 0) {
		errno = ENOENT;
		return -1;
	}

	/* DOS doesn't stat "/" correctly, so fake it here. */
	if (strcmp(path + 1, ":/") == 0) {
		buf->st_dev = 0;
		buf->st_ino = 0;
		buf->st_mode = S_IREAD | S_IWRITE | S_IFDIR;
		buf->st_uid = buf->st_gid = 0;
		buf->st_nlink = 1;
		buf->st_rdev = 0;
		buf->st_size = 0;
		buf->st_atime = buf->st_mtime = buf->st_ctime = 0;
		return 0;
	}

	/* Now modify the string so that "dir/" works. */
	len = strlen(path);
	if (path[len - 1] == '/') path[len - 1] = '\0';
	return stat(path, buf);
}


/*
 * rootpath  --  convert a pathname argument to root based cannonical form.
 *
 * rootpath determines the current directory, appends the path argument (which
 * may affect which disk the current directory is relative to), and qualifies
 * "." and ".." references.  The result is a complete, simple, path name with
 * drive specifier.
 *
 * If the relative path the user specifies does not include a drive spec., the
 * default drive will be used as the base.  (The default drive will never be
 * changed.)
 *
 *  entry: relpath  -- pointer to the pathname to be expanded
 *         fullpath -- must point to a working buffer, see warning
 *   exit: fullpath -- the full path which results
 * return: -1 if an error occurs, 0 otherwise
 *
 * warning: fullpath must point to a working buffer large enough to hold the
 *          longest possible relative path argument plus the longest possible
 *          current directory path.
 */
static int rootpath(const char *relpath, char *fullpath) {
	register char *lead, *follow, *s;
	char tempchar;
	int drivenum;

	/* Extract drive spec. */
	if ((*relpath != '\0') && (relpath[1] == ':')) {
		drivenum = toupper(*relpath) - 'A';
		relpath += 2;
	}
	else {
		drivenum = getdisk();
	}

	/* Fill in the drive path. */
	strcpy(fullpath, " :/");
	fullpath[0] = drivenum + 'A';

	/* Get cwd for drive - also checks that drive exists. */
	if (getcurdir(drivenum + 1, fullpath + 3) == -1) {
		return -1;		/* No such drive - give up. */
	}

	/* Swap all \'s for /'s. */
	s = fullpath;
	while (*s) {
		if (*s == '\\') *s = '/';
		s++;
	}

	/* Append relpath to fullpath/base. */
	if (*relpath == '/') {				/* relpath starts at base... */
		strcpy(fullpath + 2, relpath);	/* ...so ignore cwd. */
	}
	else {								/* relpath is relative to cwd. */
		if (*relpath != '\0') {
			if (strlen(fullpath) > 3) {
				strcat(fullpath, "/");	/* Add a '/' to end of cwd. */
			}
			strcat(fullpath, relpath);	/* Add relpath to end of cwd. */
		}
	}

	/* Convert path to cannonical form. */
	lead = fullpath;
	while (*lead != '\0') {
		/* Mark next path segment. */
		follow = lead;
		lead = (char *)strchr(follow + 1, '/');
		if (lead == 0) {
			lead = fullpath + strlen(fullpath);
		}
		tempchar = *lead;
		*lead = '\0';

		/* "." segment? */
		if (strcmp(follow + 1, ".") == 0) {
			*lead = tempchar;
			strcpy(follow, lead);		/* Remove "." segment. */
			lead = follow;
		}

		/* ".." segment? */
		else if (strcmp(follow + 1, "..") == 0) {
			*lead = tempchar;
			do {
				if (--follow < fullpath) {
					follow = fullpath + 2;
					break;
				}
			} while (*follow != '/');
			strcpy(follow, lead);		/* Remove ".." segment. */
			lead = follow;
		}

		/* Normal segment. */
		else {
			*lead = tempchar;
		}
	}

	if (strlen(fullpath) == 2) {		/* 'D:' or some such. */
		strcat(fullpath, "/");
	}

	/* All done. */
	return 0;
}
