/*
 *   Playlist Entry Sequence Class for Falcon Player (FPP)
 *
 *   Copyright (C) 2013-2018 the Falcon Player Developers
 *      Initial development by:
 *      - David Pitts (dpitts)
 *      - Tony Mace (MyKroFt)
 *      - Mathew Mrosko (Materdaddy)
 *      - Chris Pinkham (CaptainMurdoch)
 *      For additional credits and developers, see credits.php.
 *
 *   The Falcon Player (FPP) is free software; you can redistribute it
 *   and/or modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation; either version 2 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "fpp-pch.h"
#include "PlaylistEntrySequence.h"
#include "fseq/FSEQFile.h"

PlaylistEntrySequence::PlaylistEntrySequence(PlaylistEntryBase *parent)
  : PlaylistEntryBase(parent),
	m_duration(0),
    m_prepared(false)

{
	LogDebug(VB_PLAYLIST, "PlaylistEntrySequence::PlaylistEntrySequence()\n");

	m_type = "sequence";
}

PlaylistEntrySequence::~PlaylistEntrySequence()
{
}

/*
 *
 */
int PlaylistEntrySequence::Init(Json::Value &config)
{
	LogDebug(VB_PLAYLIST, "PlaylistEntrySequence::Init()\n");

	if (!config.isMember("sequenceName"))
	{
		LogErr(VB_PLAYLIST, "Missing sequenceName entry\n");
		return 0;
	}

	m_sequenceName = config["sequenceName"].asString();
	return PlaylistEntryBase::Init(config);
}


int PlaylistEntrySequence::PreparePlay() {
    if (sequence->OpenSequenceFile(m_sequenceName.c_str(), 0) <= 0) {
        LogErr(VB_PLAYLIST, "Error opening sequence %s\n", m_sequenceName.c_str());
        return 0;
    }
    m_prepared = true;
    m_duration = sequence->m_seqMSDuration;
    return 1;
}

/*
 *
 */
int PlaylistEntrySequence::StartPlaying(void)
{
	LogDebug(VB_PLAYLIST, "PlaylistEntrySequence::StartPlaying()\n");

	if (!CanPlay()) {
        m_prepared = false;
		FinishPlay();
		return 0;
	}
    
    if (!m_prepared) {
        PreparePlay();
    }
    
    sequence->StartSequence();

    LogDebug(VB_PLAYLIST, "Started Sequence, ID: %s\n", m_sequenceName.c_str());

	if (mqtt)
		mqtt->Publish("playlist/sequence/status", m_sequenceName);

	return PlaylistEntryBase::StartPlaying();
}

/*
 *
 */
int PlaylistEntrySequence::Process(void)
{
	if (!sequence->IsSequenceRunning())
	{
		FinishPlay();
        m_prepared = false;

		if (mqtt)
			mqtt->Publish("playlist/sequence/status", "");
	}

	return PlaylistEntryBase::Process();
}

/*
 *
 */
int PlaylistEntrySequence::Stop(void)
{
	LogDebug(VB_PLAYLIST, "PlaylistEntrySequence::Stop()\n");

	sequence->CloseSequenceFile();
    m_prepared = false;
	if (mqtt)
		mqtt->Publish("playlist/sequence/status", "");

	return PlaylistEntryBase::Stop();
}

uint64_t PlaylistEntrySequence::GetLengthInMS() {
    if (m_duration == 0) {
        std::string n = getSequenceDirectory();
        n += "/";
        n += m_sequenceName;
        if (FileExists(n)) {
            FSEQFile* fs = FSEQFile::openFSEQFile(n);
            m_duration = fs->getTotalTimeMS();
            delete fs;
        }
    }
    return m_duration;
}
uint64_t PlaylistEntrySequence::GetElapsedMS() {
    if (m_prepared) {
        return sequence->m_seqMSElapsed;
    }
    return 0;
}


/*
 *
 */
void PlaylistEntrySequence::Dump(void)
{
	PlaylistEntryBase::Dump();

	LogDebug(VB_PLAYLIST, "Sequence Filename: %s\n", m_sequenceName.c_str());
}

/*
 *
 */
Json::Value PlaylistEntrySequence::GetConfig(void)
{
	Json::Value result = PlaylistEntryBase::GetConfig();

	result["sequenceName"]     = m_sequenceName;
	result["secondsElapsed"]   = sequence->m_seqMSElapsed / 1000;
	result["secondsRemaining"] = sequence->m_seqMSRemaining / 1000;

	return result;
}
Json::Value PlaylistEntrySequence::GetMqttStatus(void) {
	Json::Value result = PlaylistEntryBase::GetMqttStatus();
	result["sequenceName"]     = m_sequenceName;
	result["secondsElapsed"]   = sequence->m_seqMSElapsed / 1000;
	result["secondsRemaining"] = sequence->m_seqMSRemaining / 1000;
	result["secondsTotal"] = sequence->m_seqMSDuration / 1000;

	return result;
}

