/* KallistiOS Ogg/Vorbis Decoder Library
 * for KOS ##version##
 *
 * sndvorbisfile.h
 * (c)2001/2002 Thorsten Titze
 *
 * Basic Ogg/Vorbis stream information and decoding routines used by
 * libsndoggvorbis. May also be used directly for playback without
 * threading.
 */

/* Typedefinition for File Information Following the Ogg-Vorbis Spec
 *
 * TITLE :Track name
 * VERSION :The version field may be used to differentiate multiple version of the
 *         same track title in a single collection. (e.g. remix info)
 * ALBUM :The collection name to which this track belongs
 * TRACKNUMBER :The track number of this piece if part of a specific larger collection
 *              or album
 * ARTIST :Track performer
 * ORGANIZATION :Name of the organization producing the track (i.e. the 'record label')
 * DESCRIPTION :A short text description of the contents
 * GENRE :A short text indication of music genre
 * DATE :Date the track was recorded
 * LOCATION :Location where track was recorded
 * COPYRIGHT :Copyright information
 * ISRC :ISRC number for the track; see {the ISRC intro page} for more information on
 *       ISRC numbers.
 *
 * (Based on v-comment.html found in original OggVorbis packages)
 */
typedef struct
{
        char    *artist;
        char    *title;
        char    *version;
        char    *album;
        char    *tracknumber;
        char    *organization;
        char    *description;
        char    *genre;
        char    *date;
        char    *location;
        char    *copyright;
        char    *isrc;
 
        const char    *filename;
 
        long    nominalbitrate;
        long    actualbitrate;
        long    actualposition;
} VorbisFile_info_t;

