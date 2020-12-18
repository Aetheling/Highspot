#include "HighspotCoding.h"
#define _USE_WINDOWS 0 // windows, linux like some different file functions

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                                            //
//                                                          Public Functions                                                                  //
//                                                                                                                                            //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CPlaylistDatabase::CPlaylistDatabase(std::string &DatabaseFile)
{
    char c;
    FILE *f;
#if _USE_WINDOWS
    fopen_s(&f, DatabaseFile.c_str(), "r");
#else
    f = fopen(DatabaseFile.c_str(), "r");
#endif
    m_nNextSongID = 1;
    m_nNextUserID = 1;
    if(f)
    {
        // scan till the first [ : marker for start of the users list
        do
        {
            c = fgetc(f);
        }
        while('[' != c);
        // read in the users
        ReadUsers(f);
        // note that the last character read by ReadUsers is the ] character closing off the user list.
        // The play lists start with the next [ character
        ScanForCharacter(1, f, '[');
        // read the play lists
        ReadPlaylists(f);
        // last, the songs -- starting with the NEXT [ character.
        ScanForCharacter(1, f, '[');
        ReadSongs(f);
        fclose(f);
    }
    else
    {
        printf("Cannot open the database file %s\n", DatabaseFile.c_str());
    }
};

void CPlaylistDatabase::ApplyChanges(std::string &ChangesFile)
{
    char                                      c, szName[1024], szTitle[8192];
    int                                       nUserID, nPlaylistID, nSongID;
    std::unordered_map<std::string,int>       mDummyLongNameTable;
    std::vector<int>                          vDummySongIDVector;
    std::unordered_map<int,std::vector<int>>  mDummyPlaylistForUser;
    FILE* f;
#if _USE_WINDOWS
    fopen_s(&f, ChangesFile.c_str(), "r");
#else
    f = fopen(ChangesFile.c_str(), "r");
#endif
    if(f)
    {
        do
        {
            // read one line -- one change command -- from the file
            c = fgetc(f);
            switch(c)
            {
                case 'A': // adding a playlist
                {
                    // format: A"user name"artist"song"artist"song ...
                    bool bLastSong = false;
                    ScanForCharacter(1, f, '"');
                    // get the user name
                    ReadQuoteClosedString(f, szName);
                    // get the user ID from the table -- or create a new one
                    nUserID     = GetOrCreateUser(szName);
                    nPlaylistID = m_mNextPlaylistForUser.find(nUserID)->second++;
                    auto UsersPlaylists = m_mPlaylists.insert(std::make_pair(nUserID, mDummyPlaylistForUser));
                    auto Playlist       = UsersPlaylists.first->second.insert(std::make_pair(nPlaylistID, vDummySongIDVector)).first;
                    // get the 
                    // read in the songs
                    do
                    {
                        // read artist
                        ReadQuoteClosedString(f, szName);
                        auto g = m_SongIDs.insert(std::make_pair(std::string(szName), mDummyLongNameTable));
                        // read the title
                        bLastSong = ReadQuoteClosedString(f, szTitle);
                        // get the song ID (or create it)
                        auto h = g.first->second.insert(std::make_pair(std::string(szTitle), m_nNextSongID));
                        if(h.second)
                        {
                            // new song
                            m_Songs.insert(std::make_pair(m_nNextSongID, std::make_pair(std::string(szName), std::string(szTitle))));
                            m_nNextSongID = m_nNextSongID+1;
                        }
                        nSongID   = h.first->second;
                        Playlist->second.push_back(nSongID);
                    }
                    while(!bLastSong);
                    break;
                }
                case 'U': // updating a playlist: add a song
                {
                    // format: U"user name"playlist ID"artist"song title
                    ScanForCharacter(1, f, '"');
                    // get user ID: first, read the user name
                    ReadQuoteClosedString(f, szName);
                    nUserID = GetOrCreateUser(szName);
                    // get the playlist ID
                    nPlaylistID = ReadID(f);
                    // get the artist
                    ReadQuoteClosedString(f, szName);
                    auto mSongsByArtist = m_SongIDs.insert(std::make_pair(std::string(szName),mDummyLongNameTable)); // might be the first mention of this artist
                    // get the song title
                    ReadQuoteClosedString(f, szTitle);
                    // get the song ID
                    auto song = mSongsByArtist.first->second.insert(std::make_pair(std::string(szTitle),1)); // might be the first mention of this song
                    if(song.second)
                    {
                        // yup; this is the first mention of this song
                        nSongID = m_nNextSongID++;
                        song.first->second = nSongID;
                        m_Songs.insert(std::make_pair(nSongID, std::make_pair(std::string(szName), std::string(szTitle))));
                    }
                    else
                    {
                        nSongID = song.first->second;
                    }
                    // add the song to the playlist for the user
                    auto user_playlists = m_mPlaylists.insert(std::make_pair(nUserID, mDummyPlaylistForUser)); // might be a new user
                    auto playlist_songs = user_playlists.first->second.insert(std::make_pair(nPlaylistID, vDummySongIDVector)); // might be a new playlist for the user
                    if(playlist_songs.second)
                    {
                        // this IS a new playlist for the user.  Update the next playlist ID available
                        auto nextsong = m_mNextPlaylistForUser.insert(std::make_pair(nUserID, 0)); // might be a new user, too
                        nextsong.first->second = nPlaylistID + 1; // whether or not it's a new user, the next playlist ID is this one + 1
                    }
                    playlist_songs.first->second.push_back(nSongID);
                    break;
                }
                case 'R': // removing a playlist
                {
                    // format: R"user name"playlist ID
                    ScanForCharacter(1, f, '"');
                    // get user ID: first, read the user name
                    ReadQuoteClosedString(f, szName);
                    // and the playlist ID
                    nPlaylistID = ReadID(f);
                    // find the user ID, and remove the playlist if found
                    auto user = m_mUserIDs.find(szName);
                    if(user != m_mUserIDs.end())
                    {
                        auto playlists = m_mPlaylists.find(user->second);
                        if(playlists != m_mPlaylists.end())
                        {
                            playlists->second.erase(nPlaylistID);
                        }
                    }
                    break;
                }
                case '#': // comment
                {
                    // ignore the line
                    ScanForCharacter(1, f, '\n');
                    break;
                }
                case EOF:
                {
                    // do nothing -- exit
                    break;
                }
                default:
                {
                    printf("Bad file format\n");
                    fclose(f);
                    return;
                }
            }
        }
        while(EOF != c);
        fclose(f);
        // now: print the revised database to the output file
#if _USE_WINDOWS
        fopen_s(&f, "output.json", "w");
#else
        f = fopen("output.json", "w");
#endif
        // first, users:
        fprintf(f, "{\n\t\"users\" : [\n");
        if(!m_mUserIDs.empty())
        {
            auto user = m_mUserIDs.begin();
            fprintf(f,"\t{\n\t\t\"id\" : \"%i\",\n\t\t\"name\" : \"%s\"\n\t}",user->second,user->first.c_str());
            user++;
            for(; user != m_mUserIDs.end(); user++)
            {
                fprintf(f,",\n\t{\n\t\t\"id\" : \"%i\",\n\t\t\"name\" : \"%s\"\n\t}",user->second,user->first.c_str());
            }
        }
        fprintf(f,"\n\t],\n");
        // next, playlists
        fprintf(f,"\"playlists\" : [\n");
        bool bFirstSong;
        if(!m_mPlaylists.empty())
        {
            bool bFirstPlaylist = true;
            for(auto userplaylists = m_mPlaylists.begin(); userplaylists != m_mPlaylists.end(); userplaylists++)
            {
                for(auto playlist = userplaylists->second.begin(); playlist != userplaylists->second.end(); playlist++)
                {
                    bFirstSong = true;
                    if(!bFirstPlaylist)
                    {
                        fprintf(f,",\n");
                    }
                    fprintf(f,"\t{\n\t\t\"id\" : \"%i\",\n\t\t\"user_id\" : \"%i\",\n\t\t\"song_ids\" : [\n",playlist->first, userplaylists->first);
                    bFirstPlaylist = false;
                    for(unsigned int i=0; i<playlist->second.size(); i++)
                    {
                        if(!bFirstSong)
                        {
                           fprintf(f,",\n");
                        }
                        bFirstSong = false;
                        fprintf(f,"\t\t\t\"%i\"",playlist->second[i]);
                    }
                    fprintf(f, "\n\t\t]\n\t}");
                }
            }
        }
        fprintf(f,"\n\t],\n");
        // Finally, print all the songs
        fprintf(f,"\t\"songs\": [\n");
        bFirstSong = true;
        for(auto ArtitistTile = m_Songs.begin(); ArtitistTile != m_Songs.end(); ArtitistTile++)
        {
            if (!bFirstSong)
            {
                fprintf(f, ",\n");
            }
            bFirstSong = false;
            fprintf(f, "\t\t{\n\t\t\t\"id\" : \"%i\",\n\t\t\t\"artist\": \"%s\",\n\t\t\t\"title\": \"%s\"\n\t\t}", ArtitistTile->first, ArtitistTile->second.first.c_str(), ArtitistTile->second.second.c_str());
        }
        fprintf(f, "\n\t]\n}\n");
    }
    else
    {
        printf("Cannot open the changes file %s\n", ChangesFile.c_str());
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                                            //
//                                                         Back-end Functions                                                                 //
//                                                                                                                                            //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CPlaylistDatabase::ReadUsers(FILE *f)
{
    char c;
    char szName[1028]; // should be more than enough space
    // Assume that we have already read the opening [ for the user list.
    // Read the users.  Note the list MIGHT be empty, so need to keep that in mind
    // Note the format is rigid: 3 " marks, then the id, 4 more " marks, then the user name, " mark, repeat -- but we might see a ] before any ", signaling the end of the list
    do
    {
        c = fgetc(f);
        if('"' == c) // reading (another) user
        {
            // find the id #
            ScanForCharacter(2, f, '"'); // already found first of 3 "
            // should be nothing but digits between the quote marks!
            int id = ReadID(f);
            if(m_nNextUserID < id) m_nNextUserID = id;
            // find the user name: follows 4 " marks (the first of which we've already read)
            ScanForCharacter(3, f, '"');
            ReadQuoteClosedString(f, szName);
            // add the name/id pair to the database
            auto f = m_mUserIDs.insert(std::make_pair(szName,id));
            if(!f.second)
            {
                printf("tried to add the same user twice -- %s\n",szName);
            }
        }
    }
    while(']' != c);
};

// Note that we have 2 song list vectors.  One is a dummy, purely for setting up the unordered map.  The other is where we read the song IDs.
// Note that we do NOT just read the IDs directly into the map: we should NOT be adding in the same song list twice!  But if we try, we shouldn't
// fail completely; we could add other song lists.  So read all the song IDs, and then add to the database if appropriate.
void CPlaylistDatabase::ReadPlaylists(FILE *f)
{
    char                                      c;
    int                                       nPlaylistID, nUserID;
    std::vector<int>                          vEmptySongList, vSongsRead;
    std::unordered_map<int,std::vector<int>>  mEmptyUserSongListSet;
    // Assumes we have already ready the opening [ of the play list.
    // Note the (rigid) format: 3 " marks, followed by id #, followed by 4 more quotes, followed by user ID, the [ followed by song IDs (likewise encased in " marks)
    do
    {
        c = fgetc(f);
        if('"' == c)
        {
            // find the playlist ID: follows 2 more "
            ScanForCharacter(2, f, '"');
            // read playlist ID
            nPlaylistID = ReadID(f);
            // Find user ID: follows 3 more "
            ScanForCharacter(3, f, '"');
            // read user ID
            nUserID = ReadID(f);
            // Update max playlist ID for the user as necessary
            auto MaxPlaylistID = m_mNextPlaylistForUser.insert(std::make_pair(nUserID, nPlaylistID));
            if(!MaxPlaylistID.second)
            {
                // user already in the system.  Update the max playlist ID as necessary
                if(MaxPlaylistID.first->second < nPlaylistID)
                {
                    MaxPlaylistID.first->second = nPlaylistID;
                }
            }
            // read the songs.  Note we are guaranteed at least 1; the first follows the user_id tag after 4 " -- the first of which we've already read
            // read 2 " marks.  Then, next song ID always follows a single " mark, regardless where we are in the list
            ScanForCharacter(2, f, '"');
            vSongsRead.resize(0);
            do
            {
                c = fgetc(f);
                if('"' == c)
                {
                    vSongsRead.push_back(ReadID(f));
                }
            }
            while(']' != c);
            // set up to add the song list
            auto user = m_mPlaylists.insert(std::make_pair(nUserID, mEmptyUserSongListSet));
            // try adding the song list
            auto songs = user.first->second.insert(std::make_pair(nPlaylistID, vEmptySongList));
            if(!songs.second)
            {
                printf("Tried to add song list %i to user %i a second time\n", nPlaylistID, nUserID);
            }
            else
            {
                songs.first->second.reserve(vSongsRead.size());
                for(unsigned int i=0; i<vSongsRead.size(); i++)
                {
                    songs.first->second.push_back(vSongsRead[i]);
                }
            }
            c = ' '; // so end-of-song-id list close isn't confused with end-of-songlist close
        }
    }
    while(']' != c);
};

void CPlaylistDatabase::ReadSongs(FILE *f)
{
    // note the format: the song ID is immediately after the third ", then 4 more " artist name, then 4 " to the title
    char                                 c;
    char                                 szArtistName[1024];
    char                                 szSongName[8192]; // hey -- the song name might be LONG for all I know.
    std::unordered_map<std::string,int>  mDummy;
    do
    {
        c = fgetc(f);
        if('"' == c)
        {
            mDummy.empty();
            // find the song ID: follows 2 more "
            ScanForCharacter(2, f, '"');
            // read song ID
            int nSongID = ReadID(f);
            if(m_nNextSongID < nSongID) m_nNextSongID = nSongID;
            // find the artist
            ScanForCharacter(3, f, '"');
            // read the artist's name
            ReadQuoteClosedString(f, szArtistName);
            // find the song name
            ScanForCharacter(3, f, '"');
            // read the artist's name
            ReadQuoteClosedString(f, szSongName);
            // update the database: indexed by artis/title
            auto f = m_SongIDs.insert(std::make_pair(std::string(szArtistName),mDummy));
            auto g = f.first->second.insert(std::make_pair(std::string(szSongName), nSongID));
            if(!g.second)
            {
                printf("Tried to add the same song twice -- %s by %s\n",szSongName, szArtistName);
            }
            else
            {
                // update the database: indexed by song ID
                m_Songs.insert(std::make_pair(nSongID, std::make_pair(std::string(szArtistName), std::string(szSongName))));
            }
        }
    }
    while(']' != c);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                                            //
//                                                         Utility Functions                                                                  //
//                                                                                                                                            //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CPlaylistDatabase::ScanForCharacter(int nNumToFind, FILE *f, char sought)
{
    while(nNumToFind)
    {
        char c = fgetc(f);
        if(sought == c) nNumToFind--;
    }
};

// assumes nothing but digits before closing "
int CPlaylistDatabase::ReadID(FILE *f)
{
    char c;
    int  id = 0;
    do
    {
        c = fgetc(f);
        if('"' == c || '\n' == c) break;
        id = 10*id + (c - '0');
    }
    while(true);
    return id;
};

bool CPlaylistDatabase::ReadQuoteClosedString(FILE *f, char *szDestination)
{
    char c;
    int  i = 0;
    do
    {
        c = fgetc(f);
        if('\n' == c || '"' == c) break;
        szDestination[i++] = c;
    }
    while(true);
    // close off name
    szDestination[i] = '\0';
    return ('\n' == c);
};

int CPlaylistDatabase::GetOrCreateUser(char *szName)
{
    int         nUserID;
    std::string sName(szName);
    auto f = m_mUserIDs.insert(std::make_pair(std::string(szName), m_nNextUserID+1));
    if(f.second)
    {
        // new user; update the max user ID and pick a playlist ID to start from
        nUserID     = m_nNextUserID++;
        m_mNextPlaylistForUser.insert(std::make_pair(nUserID,1));
    }
    else
    {
        // existing user.  Get the next available playlist ID
        nUserID     = f.first->second;
    }
    return nUserID;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                                            //
//                                                           Program entry                                                                    //
//                                                                                                                                            //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main()
{
    std::string sPlaylistName("mixtape.json");
    std::string sChangelistName("changes.txt");
    CPlaylistDatabase mPlaylist(sPlaylistName);
    mPlaylist.ApplyChanges(sChangelistName);
    return 0;
}
