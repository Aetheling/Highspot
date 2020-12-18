For compilation, my Windows box and Linux box like different file opening functions; set the _USE_WINDOWS flag accordingly (line 2 of
HighspotCoding.cpp; it defaults to the Linux version).
To run, just make sure the input file -- hardwired to mixtape.json -- and the changes file, likewise hardcoded (changes.txt) are in the
same folder as the executable.

Scaling for large databases:

First, the input file.  The input file was given a format to make it human-readable; this includes lots of keywords to help the user find
the information needed.  If the input file becomes very large, it will not be human readable simply due to scale -- a million songs would
be far from a lot of data for the computer; it is far too much for a human to read.  Parsing the input file would be simpler and faster if
everything is given by position, rather than with keywords (as the changes file does).  This would have the side effect of making the file
smaller.  To deal with the human readability issue, I would add more accessor functions:
* Print all songs by a given artist
* Give the artist for a given song title (or list of artists, if multiple artists have songs with the same name)
* Give all playlists for a user as songs-by-artists, rather than as IDs -- e.g. " 'Too cool to write' by 'Technophobe', 'Math is for Losers' by 'Technophobe', 'Evil Empire' by 'Lost Boys', 'I Just Flunked English' by 'Technophobe'

If the input file really is gargantuan -- by computer standards, not just from a user perspective -- breaking up the database may be necessary.
Note that the input file itself isn't the problem (it's read linearly); rather, the amount of memory needed to keep track of everything to make
it available is.  The database has two components: the users/user playlists, and the songs.  In all likelihood there will be far more users to
keep track of than artists, so partitioning the users into separate databases makes sense.  Unless there is reason to expect questions such as
"which users listen to Nirvana?" as opposed to "What songs does John listen to?" we will need (potentially) all of the songs for each user, but
not all of the users for each song.  But since the song database is likely much smaller than the user database, loading it up each time along
with a segment of the users should be feasible.

With this, the changes.txt file should likewise be partitioned based on users (using the same partitions as for the input file, of course).  If
either the changes file or (more likely) the input file become "too large" -- the database is too big to work with easily in memory -- partition
the users further.

The partitioning could be done automatically whenever the user database grows too large -- say, when a partition grows to over a million users,
split it in half.  Keep the songs/artists in one file; a partitioning file with the split points (a small file -- the break points between the
partitions, so one fewer elements than there are partitions); and the user files, one per partition.  For the large partition, when the time
comes to write the output pick the midpoint of its users, insert that into the partioning file, and write the partition as two partitions instead
of one: those users that fall at or below the split point, and those above.