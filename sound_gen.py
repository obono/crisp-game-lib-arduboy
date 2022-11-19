#!/usr/bin/python
import math
import sys
import time

#----- Random

class MyRandom:

	UINT32_MAX = 0xFFFFFFFF

	def __init__(self, seed=None):
		self.setRandomSeed(time.time() if seed is None else seed)

	def nextRandom(self):
		t = (self.x ^ (self.x << 11)) & self.UINT32_MAX
		self.x = self.y
		self.y = self.z
		self.z = self.w
		self.w = (self.w ^ (self.w >> 19) ^ (t ^ (t >> 8))) & self.UINT32_MAX
		return self.w

	def getRandom(self, low, high):
		return self.nextRandom() / self.UINT32_MAX * (high - low) + low

	def getIntRandom(self, low, high):
		if low == high:
			return low
		return self.nextRandom() % (high - low) + low

	def getPlusOrMinusRandom(self):
		return self.getIntRandom(0, 2) * 2 - 1

	def setRandomSeed(self, w):
		loopCount = 32
		self.x = 123456789
		self.y = 362436069
		self.z = 521288629
		self.w = int(w) & self.UINT32_MAX
		for i in range(loopCount):
			self.nextRandom()

r = MyRandom()

def _rndi(low, high):
	return r.getIntRandom(low, high)

def _rnd(low, high):
	return r.getRandom(low, high)

#----- Common

def addNote(notes, midiNote, when, duration):
	notes.append({'midiNote': midiNote, 'when': when, 'duration': duration})

def addNotes(notes, count, when, keyFrom, keyTo, amplitudeFrom, amplitudeTo):
	mn = keyFrom
	mo = (keyTo - keyFrom) / (count - 1)
	an = amplitudeFrom
	ao = (amplitudeTo - amplitudeFrom) / (count - 1)
	for i in range(count):
		addNote(notes, int(mn + _rnd(-an, an)), when, 1)
		mn += mo
		an += ao
		when += 1

#----- SE

def coinSe():
	notes = []
	w = 0
	d = _rndi(4, 8)
	addNote(notes, _rndi(70, 80), w, d)
	w += d
	d = _rndi(7, 15)
	addNote(notes, _rndi(85, 95), w, d)
	return notes

def laserSe():
	notes = []
	w = 0
	d = _rndi(9, 19)
	addNotes(notes, d, w, _rndi(75, 95), _rndi(45, 65), _rndi(5, 9), 0)
	return notes

def explosionSe():
	notes = []
	w = 0
	d = _rndi(5, 12)
	addNotes(notes, d, w, _rndi(70, 90), _rndi(50, 60), _rndi(5, 15), _rndi(15, 25))
	w += d
	d = _rndi(12, 20)
	addNotes(notes, d, w, _rndi(50, 70), _rndi(30, 50), _rndi(15, 25), _rndi(5, 15))
	return notes

def powerUpSe():
	notes = []
	w = 0
	d = _rndi(2, 5)
	addNotes(notes, d, w, _rndi(75, 85), _rndi(65, 75), 0, 0)
	w += d
	d = _rndi(6, 9)
	addNotes(notes, d, w, _rndi(65, 75), _rndi(85, 95), 0, 0)
	w += d
	d = _rndi(3, 6)
	addNotes(notes, d, w, _rndi(85, 95), _rndi(75, 85), 0, 0)
	w += d
	d = _rndi(6, 9)
	addNotes(notes, d, w, _rndi(75, 85), _rndi(95, 105), 0, 0)
	return notes

def hitSe():
	notes = []
	w = 0
	d = _rndi(5, 9)
	addNotes(notes, d, w, _rndi(70, 80), _rndi(60, 70), _rndi(0, 10), _rndi(0, 10))
	return notes

def jumpSe():
	notes = []
	w = 0
	d = _rndi(2, 5)
	addNote(notes, _rndi(70, 80), w, d)
	w += d
	d = _rndi(9, 19)
	addNotes(notes, d, w, _rndi(55, 70), _rndi(80, 95), 0, 0)
	return notes

def selectSe():
	notes = []
	f = _rndi(60, 90)
	w = 0
	d = _rndi(2, 4)
	addNote(notes, f, w, math.ceil(d * 0.7))
	w += d
	d = _rndi(4, 9)
	f += _rndi(-2, 5)
	addNote(notes, f, w, d)
	return notes

def randomSe():
	notes = []
	w = 0
	d = _rndi(3, 15)
	addNotes(notes, d, w, _rndi(30, 99), _rndi(30, 99), _rndi(0, 20), _rndi(0, 20))
	w += d
	d = _rndi(3, 15)
	addNotes(notes, d, w, _rndi(30, 99), _rndi(30, 99), _rndi(0, 20), _rndi(0, 20))
	return notes

def clickSe():
	notes = []
	w = 0
	d = _rndi(2, 6)
	addNote(notes, _rndi(65, 80), w, math.ceil(d / 2))
	w += d
	d = _rndi(2, 6)
	addNote(notes, _rndi(70, 85), w, math.ceil(d / 2))
	return notes

#----- BGM

CHORDS = [[
	{'midiNote': 0, 'isMinor': False},
	{'midiNote': 0, 'isMinor': False},
	{'midiNote': 4, 'isMinor': True},
	{'midiNote': 9, 'isMinor': True},
],[
	{'midiNote': 5, 'isMinor': False},
	{'midiNote': 5, 'isMinor': False},
	{'midiNote': 2, 'isMinor': True},
	{'midiNote': 2, 'isMinor': True},
],[
	{'midiNote': 7, 'isMinor': False},
	{'midiNote': 7, 'isMinor': False},
	{'midiNote': 11, 'isMinor': True},
	{'midiNote': 11, 'isMinor': True},
]]

NEXT_CHORDS_INDEX = [[0, 1, 2], [1, 2, 0], [2, 0, 0]]

KEYS = [0, 2, 3, 5, 7, 9, 10]
PROGRESSION = [[0, 4, 7, 11], [0, 3, 7, 10]]

def generateChordProgression(len):
	key = KEYS[_rndi(0, 7)]
	octave = 3
	chordChangeInterval = 4
	chord = None
	chordIndex = 0
	midiNotes = [[0 for j in range(4)] for i in range(len)]
	for i in range(len):
		if i % chordChangeInterval == 0:
			if i == 0:
				chordIndex = _rndi(0, 2)
				chord = CHORDS[chordIndex][_rndi(0, 4)]
			elif _rnd(0, 1) < 0.8 - ((i / chordChangeInterval) % 2 * 0.5):
				chordIndex = NEXT_CHORDS_INDEX[chordIndex][_rndi(0, 3)]
				chord = CHORDS[chordIndex][_rndi(0, 4)]
		pi = 1 if chord['isMinor'] else 0
		for j in range(4):
			midiNotes[i][j] = octave * 12 + 12 + key + chord['midiNote'] + PROGRESSION[pi][j]
	return midiNotes

def reversePattern(pattern, interval, length, freq):
	pt = [False for i in range(interval)]
	for i in range(freq):
		pt[_rndi(0, interval)] = True
	for i in range(length):
		if pt[i % interval]:
			pattern[i] = not pattern[i]

def createRandomPattern(length, freq):
	interval = 4
	pattern = [False for i in range(length)]
	while interval < length:
		reversePattern(pattern, interval, length, freq)
		interval *= 2
	return pattern

def generateBgm():
	noteLength = 32 # bgmNoteLength
	chordMidiNotes = generateChordProgression(noteLength)
	pattern = createRandomPattern(noteLength, 1)
	continuingPattern = [_rnd(0, 1) < 0.8 for i in range(noteLength)]
	duration = 8
	when = -duration
	hasPrevNote = False
	notes = []
	for i in range(noteLength):
		when += duration
		if pattern[i] == False:
			hasPrevNote = False
			continue
		if continuingPattern[i] and hasPrevNote:
			notes[-1]['duration'] += duration
			continue
		hasPrevNote = True
		addNote(notes, chordMidiNotes[i][_rndi(0, 4)], when, duration)
	return notes

#----- Converter

BASE_NOTE_DURATION = 1000 / 60

def convertData(notes, ch=0, rep=False):
	data = []
	w = 0
	for note in notes:
		if w < note['when']:
			data.append(0x80 + ch)
			d = note['when'] - w
			appendWaitData(data, w, d)
			w += d
		data.append(0x90 + ch)
		data.append(note['midiNote'])
		d = note['duration']
		appendWaitData(data, w, d)
		w += d
	data.append(0x80 + ch)
	data.append(0xE0 if rep else 0xF0)
	return data

def appendWaitData(data, w, d):
	ms = int((w+d)*BASE_NOTE_DURATION) - int(w*BASE_NOTE_DURATION) 
	data.append((ms >> 8) & 0x7F)
	data.append(ms & 0xFF)


def outputData(name, data):
	print('PROGMEM static const uint8_t ' + name + '[] = {')
	for i, b in enumerate(data):
		if i % 8 == 0:
			print('  ', end='')
		delim = '\n' if i % 8 == 7 else ' '
		print('0x' + format(b, '02X') + ',', end=delim)
	if len(data) % 8 > 0:
		print()
	print('};')
	print()

def getHashFromString(title, desc, extra=0):
	hash = 0
	str = title + desc
	if len(str) > 98:
		str = str[:98]
	for c in str:
		hash = (hash << 5) - hash + ord(c)
		hash &= 0xFFFFFFFF
	return hash + extra

#----- Main

if __name__ == '__main__':
	seed = r.nextRandom() if len(sys.argv) < 2 else int(sys.argv[1])
	r.setRandomSeed(seed)
	print('// This code is generated by \'sound_gen.py\' (seed = ' + str(seed) + ')')
	print()
	sounds = {
		'coinSe': coinSe(),
		'laserSe': laserSe(),
		'explosionSe': explosionSe(),
		'powerUpSe': powerUpSe(),
		'hitSe': hitSe(),
		'jumpSe': jumpSe(),
		'selectSe': selectSe(),
		'randomSe': randomSe(),
		'clickSe': clickSe(),
		'bgm': generateBgm(),
	}
	print('#pragma once')
	print()
	for name, note in sounds.items():
		repeat = (name == 'bgm')
		data = convertData(note, rep=repeat)
		outputData(name, data)
	print('PROGMEM static const uint8_t * const soundPatterns[] = {')
	print('  ' + ',\n  '.join(sounds.keys()) + ',')
	print('};')
