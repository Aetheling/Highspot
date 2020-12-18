#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utility>
#include <unordered_map>
#include <map>
#include <vector>

class CPlaylistDatabase
{
public:  // functions
    CPlaylistDatabase(std::string &DatabaseFile); // .json file
    void ApplyChanges(std::string &ChangesFile);  // .txt file
private: // functions
    // backend functions
    void ReadUsers(FILE *f);
    void ReadPlaylists(FILE *f);
    void ReadSongs(FILE *f);
    // utility functions
    void ScanForCharacter(int nNumQuotesToRead, FILE *f, char sought);
    int  ReadID(FILE *f);
    bool ReadQuoteClosedString(FILE *f, char *szDestination); // or terminating in end-of-line.  Returns true if ends with end-of-line, false otherwise
    int  GetOrCreateUser(char *szName);
public:  // variables
private: // variables
    std::unordered_map<std::string,int>                                 m_mUserIDs;
    std::unordered_map<std::string,std::unordered_map<std::string,int>> m_SongIDs;    // indexed by artist, title
    std::map<int,std::pair<std::string,std::string>>                    m_Songs;      // indexed by song ID: artist, song
    std::unordered_map<int,std::unordered_map<int,std::vector<int>>>    m_mPlaylists; // indexed by user id, playlist id; vector of song IDs.  Note that we allow the same song to appear multiple times in the playlist
    std::unordered_map<int,int>                                         m_mNextPlaylistForUser; // for each user, the playlist ID to be used next.  For creating new playlists -- want a NEW id.
    int                                                                 m_nNextUserID;          // to make it easy to get a new ID when a user is added
    int                                                                 m_nNextSongID;          // to make it easy to get a new ID when a song is added
};
    
    