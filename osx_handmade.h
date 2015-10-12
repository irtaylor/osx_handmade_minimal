#pragma once

#include <AudioUnit/AudioUnit.h>
#include <stdint.h>
#include <math.h>
#include <dispatch/dispatch.h>
#include <glob.h>

struct osx_offscreen_buffer
{
	// NOTE: Pixels are always 32-bits wide. BB GG RR XX
	//BITMAPINFO Info;
	void* Memory;
	int Width;
	int Height;
	int Pitch;
	int BytesPerPixel;
};


struct osx_window_dimension
{
	int Width;
	int Height;
};


struct osx_game_code
{
	void* GameCodeDL;
	time_t DLLastWriteTime;

    // IMPORTANT(casey): Either of the callbacks can be 0!  You must
    // check before calling.
	game_update_and_render* UpdateAndRender;
    game_get_sound_samples *GetSoundSamples;

    bool32 IsValid;
};


struct osx_sound_output
{
	game_sound_output_buffer SoundBuffer;
	uint32 SoundBufferSize;
	int16* CoreAudioBuffer;
	int16* ReadCursor;
	int16* WriteCursor;

	AudioStreamBasicDescription AudioDescriptor;
	AudioUnit AudioUnit;

#if 0
	AudioQueueRef AudioQueue;
	AudioQueueBufferRef AudioBuffers[2];
	bool32 RanAFrame;
#endif
};


//#define OSX_STATE_FILENAME_COUNT PROC_PIDPATHINFO_MAXSIZE
#define OSX_STATE_FILENAME_COUNT FILENAME_MAX

struct osx_replay_buffer
{
	int FileHandle;
	int MemoryMap;
	char Filename[OSX_STATE_FILENAME_COUNT];
	void* MemoryBlock;
};


struct osx_state
{
	uint64 TotalSize;
	void* GameMemoryBlock;
	osx_replay_buffer ReplayBuffers[4];

	int RecordingHandle;
	int InputRecordingIndex;

	int PlaybackHandle;
	int InputPlayingIndex;

	char AppFilename[OSX_STATE_FILENAME_COUNT];
	char* OnePastLastAppFilenameSlash;
};


#ifndef HANDMADE_USE_ASM_RDTSC
// NOTE(jeff): Thanks to @visitect for this suggestion
#define rdtsc __builtin_readcyclecounter
#else
internal inline uint64
rdtsc()
{
	uint32 eax = 0;
	uint32 edx;

	__asm__ __volatile__("cpuid;"
			     "rdtsc;"
				: "+a" (eax), "=d" (edx)
				:
				: "%rcx", "%rbx", "memory");

	__asm__ __volatile__("xorl %%eax, %%eax;"
			     "cpuid;"
				:
				:
				: "%rax", "%rbx", "%rcx", "%rdx", "memory");

	return (((uint64)edx << 32) | eax);
}
#endif


void OSXGetAppFilename(osx_state *State);

void OSXBuildAppPathFilename(osx_state *State, char *Filename,
                             int DestCount, char *Dest);

DEBUG_PLATFORM_FREE_FILE_MEMORY(DEBUGPlatformFreeFileMemory);
DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFile);
DEBUG_PLATFORM_WRITE_ENTIRE_FILE(DEBUGPlatformWriteEntireFile);


struct osx_platform_file_handle
{
	platform_file_handle H;
	int OSXFileHandle;
	char Filename[256];
};


struct osx_platform_file_group
{
	platform_file_group H;
	glob_t GlobResults;
	int CurrentIndex;
};


PLATFORM_GET_ALL_FILE_OF_TYPE_BEGIN(OSXGetAllFilesOfTypeBegin);
PLATFORM_GET_ALL_FILE_OF_TYPE_END(OSXGetAllFilesOfTypeEnd);
PLATFORM_OPEN_FILE(OSXOpenNextFile);
PLATFORM_READ_DATA_FROM_FILE(OSXReadDataFromFile);
PLATFORM_FILE_ERROR(OSXFileError);


time_t OSXGetLastWriteTime(const char* Filename);

osx_game_code OSXLoadGameCode(const char* SourceDLName);
void OSXUnloadGameCode(osx_game_code* GameCode);


void OSXGetInputFileLocation(osx_state* State, bool32 InputStream, int SlotIndex, int DestCount, char* Dest);
void OSXBeginRecordingInput(osx_state* State, int InputRecordingIndex);
void OSXEndRecordingInput(osx_state* State);
void OSXBeginInputPlayback(osx_state* State, int InputPlayingIndex);
void OSXEndInputPlayback(osx_state* State);
void OSXRecordInput(osx_state* State, game_input* NewInput);
void OSXPlaybackInput(osx_state* State, game_input* NewInput);


struct platform_work_queue_entry
{
	platform_work_queue_callback* Callback;
	void* Data;
};


struct platform_work_queue
{
	uint32 volatile CompletionGoal;
	uint32 volatile CompletionCount;
    
	uint32 volatile NextEntryToWrite;
	uint32 volatile NextEntryToRead;
	dispatch_semaphore_t SemaphoreHandle;

	platform_work_queue_entry Entries[256];
};


struct osx_thread_info
{
    int LogicalThreadIndex;
    platform_work_queue *Queue;
};


void OSXMakeQueue(platform_work_queue* Queue, uint32 ThreadCount);
void OSXAddEntry(platform_work_queue* Queue, platform_work_queue_callback* Callback, void* Data);
bool32 OSXDoNextWorkQueueEntry(platform_work_queue* Queue);
void OSXCompleteAllWork(platform_work_queue *Queue);



