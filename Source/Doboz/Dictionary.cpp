/*
 * Doboz Data Compression Library
 * Copyright (C) 2010-2011 Attila T. Afra <attila.afra@gmail.com>
 * 
 * This program is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along with this program. If
 * not, see <http://www.gnu.org/licenses/>.
 */

#include <cassert>
#include <algorithm>
#include "Dictionary.h"

namespace doboz {
namespace detail {

Dictionary::Dictionary()
{
	assert(INVALID_POSITION < 0);
	assert(REBASE_THRESHOLD > DICTIONARY_SIZE && REBASE_THRESHOLD % DICTIONARY_SIZE == 0);

	hashTable_ = 0;
	children_ = 0;
}

Dictionary::~Dictionary()
{
	destroy();
}

void Dictionary::init()
{
	// Release the previously allocated resources
	destroy();

	// Create the hash table
	hashTable_ = new int[HASH_TABLE_SIZE];

	// Create the tree nodes
	// The number of nodes is equal to the size of the dictionary, and every node has two children
	children_ = new int[CHILD_COUNT];
}

void Dictionary::destroy()
{
	delete[] hashTable_;
	hashTable_ = 0;

	delete[] children_;
	children_ = 0;
}

void Dictionary::setBuffer(const uint8_t* buffer, size_t bufferLength)
{
	// Set the buffer
	buffer_ = buffer;
	bufferLength_ = bufferLength;
	absolutePosition_ = 0;

	// Compute the matchable buffer length
	if (bufferLength_ > TAIL_LENGTH + MIN_MATCH_LENGTH)
	{
		matchableBufferLength_ = bufferLength_ - (TAIL_LENGTH + MIN_MATCH_LENGTH);
	}
	else
	{
		matchableBufferLength_ = 0;
	}

	// Since we always store 32-bit positions in the dictionary, we need relative positions in order to support buffers larger then 2 GB
	// This can be possible, because the difference between any two positions stored in the dictionary never exceeds the size of the dictionary
	// We don't store larger (64-bit) positions, because that can significantly degrade performance
	// Initialize the relative position base pointer
	bufferBase_ = buffer_;
	
	// Initialize, if necessary
	if (hashTable_ == 0)
	{
		init();
	}

	// Clear the hash table
	for (int i = 0; i < HASH_TABLE_SIZE; ++i)
	{
		hashTable_[i] = INVALID_POSITION;
	}
}

// Finds match candidates at the current buffer position and slides the matching window to the next character
// Call findMatches/update with increasing positions
// The match candidates are stored in the supplied array, ordered by their length (ascending)
// The return value is the number of match candidates in the array
int Dictionary::findMatches(Match* matchCandidates)
{
	assert(hashTable_ != 0 && "No buffer is set.");

	// Check whether we can find matches at this position
	if (absolutePosition_ >= matchableBufferLength_)
	{
		// Slide the matching window with one character
		++absolutePosition_;

		return 0;
	}

	// Compute the maximum match length
	int maxMatchLength = static_cast<int>(std::min(bufferLength_ - TAIL_LENGTH - absolutePosition_, static_cast<size_t>(MAX_MATCH_LENGTH)));

	// Compute the position relative to the beginning of bufferBase_
	// All other positions (including the ones stored in the hash table and the binary trees) are relative too
	// From now on, we can safely ignore this position technique
	int position = computeRelativePosition();

	// Compute the minimum match position
	int minMatchPosition = (position < DICTIONARY_SIZE) ? 0 : (position - DICTIONARY_SIZE + 1);

	// Compute the hash value for the current string
	int hashValue = hash(bufferBase_ + position) % HASH_TABLE_SIZE;

	// Get the position of the first match from the hash table
	int matchPosition = hashTable_[hashValue];

	// Set the current string as the root of the binary tree corresponding to the hash table entry
	hashTable_[hashValue] = position;

	// Compute the current cyclic position in the dictionary
	int cyclicInputPosition = position % DICTIONARY_SIZE;

	// Initialize the references to the leaves of the new root's left and right subtrees
	int leftSubtreeLeaf = cyclicInputPosition * 2;
	int rightSubtreeLeaf = cyclicInputPosition * 2 + 1;

	// Initialize the match lenghts of the lower and upper bounds of the current string (lowMatch < match < highMatch)
	// We use these to avoid unneccesary character comparisons at the beginnings of the strings
	int lowMatchLength = 0;
	int highMatchLength = 0;

	// Initialize the longest match length
	int longestMatchLength = 0;

	// Find matches
	// We look for the current string in the binary search tree and we rebuild the tree at the same time
	// The deeper a node is in the tree, the lower is its position, so the root is the string with the highest position (lowest offset)

	// We count the number of match attempts, and exit if it has reached a certain threshold
	int matchCount = 0;

	// Match candidates are matches which are longer than any previously encountered ones
	int matchCandidateCount = 0;

	for (; ;)
	{
		// Check whether the current match position is valid
		if (matchPosition < minMatchPosition || matchCount == MAX_MATCH_CANDIDATE_COUNT)
		{
			// We have checked all valid matches, so finish the new tree and exit
			children_[leftSubtreeLeaf] = INVALID_POSITION;
			children_[rightSubtreeLeaf] = INVALID_POSITION;
			break;
		}

		++matchCount;

		// Compute the cyclic position of the current match in the dictionary
		int cyclicMatchPosition = matchPosition % DICTIONARY_SIZE;

		// Use the match lengths of the low and high bounds to determine the number of characters that surely match
		int matchLength = std::min(lowMatchLength, highMatchLength);

		// Determine the match length
		while (matchLength < maxMatchLength && bufferBase_[position + matchLength] == bufferBase_[matchPosition + matchLength])
		{
			++matchLength;
		}

		// Check whether this match is the longest so far
		int matchOffset = position - matchPosition;

		if (matchLength > longestMatchLength && matchLength >= MIN_MATCH_LENGTH)
		{
			longestMatchLength = matchLength;

			// Add the current best match to the list of good match candidates
			if (matchCandidates != 0)
			{
				matchCandidates[matchCandidateCount].length = matchLength;
				matchCandidates[matchCandidateCount].offset = matchOffset;
				++matchCandidateCount;
			}

			// If the match length is the maximum allowed value, the current string is already inserted into the tree: the current node
			if (matchLength == maxMatchLength)
			{
				// Since the current string is also the root of the tree, delete the current node
				children_[leftSubtreeLeaf] = children_[cyclicMatchPosition * 2];
				children_[rightSubtreeLeaf] = children_[cyclicMatchPosition * 2 + 1];
				break;
			}
		}

		// Compare the two strings
		if (bufferBase_[position + matchLength] < bufferBase_[matchPosition + matchLength])
		{
			// Insert the matched string into the right subtree
			children_[rightSubtreeLeaf] = matchPosition;

			// Go left
			rightSubtreeLeaf = cyclicMatchPosition * 2;
			matchPosition = children_[rightSubtreeLeaf];

			// Update the match length of the high bound
			highMatchLength = matchLength;
		}
		else
		{
			// Insert the matched string into the left subtree
			children_[leftSubtreeLeaf] = matchPosition;

			// Go right
			leftSubtreeLeaf = cyclicMatchPosition * 2 + 1;
			matchPosition = children_[leftSubtreeLeaf];

			// Update the match length of the low bound
			lowMatchLength = matchLength;
		}
	}

	// Slide the matching window with one character
	++absolutePosition_;

	return matchCandidateCount;
}

// Increments the match window position with one character
int Dictionary::computeRelativePosition()
{
	int position = static_cast<int>(absolutePosition_ - (bufferBase_ - buffer_));

	// Check whether the current position has reached the rebase threshold
	if (position == REBASE_THRESHOLD)
	{
		// Rebase
		int rebaseDelta = REBASE_THRESHOLD - DICTIONARY_SIZE;
		assert(rebaseDelta % DICTIONARY_SIZE == 0);

		bufferBase_ += rebaseDelta;
		position -= rebaseDelta;

		// Rebase the hash entries
		for (int i = 0; i < HASH_TABLE_SIZE; ++i)
		{
			hashTable_[i] = (hashTable_[i] >= rebaseDelta) ? (hashTable_[i] - rebaseDelta) : INVALID_POSITION;
		}

		// Rebase the binary tree nodes
		for (int i = 0; i < CHILD_COUNT; ++i)
		{
			children_[i] = (children_[i] >= rebaseDelta) ? (children_[i] - rebaseDelta) : INVALID_POSITION;
		}
	}
	
	return position;
}

// Slides the matching window to the next character without looking for matches, but it still has to update the dictionary
void Dictionary::skip()
{
	findMatches(0);
}

uint32_t Dictionary::hash(const uint8_t* data)
{
	// FNV-1a hash
	const uint32_t prime = 16777619;
	uint32_t result = 2166136261;

	result = (result ^ data[0]) * prime;
	result = (result ^ data[1]) * prime;
	result = (result ^ data[2]) * prime;

	return result;
}

} // namespace detail
} // namespace doboz