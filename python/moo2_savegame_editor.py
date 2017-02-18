##
## TODO:
##   figure out if the "NO CONTACT" race-state can be reset
## NOTE:
##   RP's are recalculated per turn and on-the-fly as soon as any colonists
##   are moved to new jobs; same is true for income (amount of BC's added to 
##   treasury each turn)
##

import os
import sys
import hashlib

## saves must be named "SAVExx.GAM" or MOO2 does not recognize
## them; digest is SHA2-256 value of the MOO2 v1.31 executable
## (ORION95.EXE, win32)
DEF_SAVE_NAME = "SAVE10.GAM"
BIN_FILE_NAME = "MOO2W32ALT.EXE"
BIN_FILE_HASH = "4bdf42ff314ab86592923dc6a18cbbdbb12ffeb21e203b97c9d28d5360e34428"
DEF_CONF_NAME = "MOX.SET"
HOF_FILE_NAME = "HOF.M2"

EDIT_PLAYER_STATS = True
EDIT_AI_RELATIONS = False

RACE_STANCE_TYPES = {
	"STANCE_DEFAULT":   0x0, ## no treaty (first-contact)
	"STANCE_NAGGPACT":  0x1,
	"STANCE_ALLIANCE":  0x2,
	"STANCE_ATPEACE":   0x3, ## peace treaty (post-war)
	"STANCE_LIMWAR":    0x4, ## "limited war" (unused)
	"STANCE_WAR":       0x5,
}

RACE_DISPOS_TYPES = {
	"DISPOS_NEUTRAL":  0x00, ## 0x{0,..,6} tested
	"DISPOS_RELAXED":  0x0f, ## 0x{a,..,f} tested
	"DISPOS_AMIABLE":  0x1f,
	"DISPOS_AFFABLE":  0x2f,
	"DISPOS_PEACEFUL": 0x3f,
	"DISPOS_FRIENDLY": 0x4f,
	"DISPOS_UNITY":    0x57,
	"DISPOS_HARMONY":  0x5f, ## 0x{5,6,7}f tested

	## note: why would FEUD be represented by 0x9f?
	## {0x80,0x8f}: FEUD but bar {in middle, at top}
	"DISPOS_FEUD":     0x9f,
	"DISPOS_HATE":     0xaf,
	"DISPOS_TROUBLED": 0xbf,
	"DISPOS_TENSE":    0xcf,
	"DISPOS_RESTLESS": 0xdf,
	"DISPOS_WARY":     0xe7,
	"DISPOS_UNEASE":   0xef,
}

RACE_TRIBUT_TYPES = {
	"MIN_PERCENT_TRIBUTE": 0x1, ##  5%
	"MAX_PERCENT_TRIBUTE": 0x2, ## 10%
}

RACE_GOVERNMENT_TYPES = {
	"GOV_FEUDAL": 0x0,
	"GOV_CONFED": 0x1,
	"GOV_DICTAT": 0x2,
	"GOV_IMPERI": 0x3,
	"GOV_DEMOCR": 0x4,
	"GOV_FEDERA": 0x5,
	"GOV_UNIFIC": 0x6,
	"GOV_GALUNI": 0x7,
}

PLAYER_DATA = {
	## can edit this for even higher score, but messes up the history-graphs
	## format of entries is (size in bytes, address, desired value, read-only)
	"STARDATE": (2, 0x00000029, 35000, True),

	## NOTE:
	##   setting amount of BC's to (1<<31)-1 will cause instant overflow
	##   at positive income, so treasury-value bytes are a *signed* int32
	##
	##   research-point bytes are interpreted as a *signed* int16!
	##
	"NUM_BCS": (4, 0x0001aa40, 2000*1000*1000,  True),
	"NUM_RPS": (2, 0x0001aaba,          32000, False),

	## clear the cheat-flag in case player used ALT + CRUNCH, etc
	## effect of this is easy to test: just wipe HOF and surrender
	##
	## cheat-codes:
	##   EINSTEIN (all technologies known)
	##   CRUNCH (complete colony build-project)
	##   MOOLA (+1000BC)
	##   MENLO (complete research-project)
	##   ISEEALL (duh)
	##   ALLAI (nothing?)
	##   SCORE (crash?)
	##
	## NOTE:
	##   ISEEALL actually changes our race-statistics, which also
	##   means it cuts into points given by Evolutionary Mutation
	##
	##   DO NOT RESEARCH EV.MUT. IF PLAYING AS AN UBER-RACE SINCE
	##   PICK-SCORE WILL BE MASSIVELY NEGATIVE (MEANS AVOIDING THE
	##   TECH-GROUP ENTIRELY IF CREATIVE) WHICH CAUSES THE MENU TO
	##   BECOME UNESCAPABLE
	##
	##   "Unification" removes morale-related colony buildings from
	##   options but "Tolerant" does *not* remove pollution-related
	##   buildings
	##
	##   if save is edited such that a race starts with an advanced
	##   government, the "Advanced Governments" research option will
	##   still be the advanced version of whatever type of government
	##   the race has by default / was assigned during customization
	"IGNORE_AMBASSADOR_1": (1, 0x0001b86e, 0x4,  True),
	"PLAYER_CHEATED_FLAG": (1, 0x0001b877, 0x0, False),

	"ATTRIB_GOVERNMENT": (1, 0x0001b2ad, RACE_GOVERNMENT_TYPES["GOV_GALUNI"], False),

	"ATTRIB_POPGROW_BONUS":   (1, 0x0001b2ae, 0x2, False), ## {0x0 = +0, 0x1 = +1, 0x2 = +2}
	"ATTRIB_FARMING_BONUS":   (1, 0x0001b2af, 0x1, False), ## {0x0 = +0, 0x1 = +2, 0x2 = +1} ??
	"ATTRIB_INDUSTRY_BONUS":  (1, 0x0001b2b0, 0x2, False), ## {0x0 = +0, 0x1 = +1, 0x2 = +2}
	"ATTRIB_RESEARCH_BONUS":  (1, 0x0001b2b1, 0x2, False), ## {0x0 = +0, 0x1 = +1, 0x2 = +2}
	"ATTRIB_INCOME_BONUS":    (1, 0x0001b2b2, 0x2, False), ## {0x0 = +0, 0x1 = +1, 0x2 = +2}
	"ATTRIB_SHIP_DEF_BONUS":  (1, 0x0001b2b3, 0x2, False), ## {0x0 = +0, 0x1 = +1, 0x2 = +2}
	"ATTRIB_SHIP_ATT_BONUS":  (1, 0x0001b2b4, 0x2, False), ## {0x0 = +0, 0x1 = +1, 0x2 = +2}
	"ATTRIB_G_COMBAT_BONUS":  (1, 0x0001b2b5, 0x2, False), ## {0x0 = +0, 0x1 = +1, 0x2 = +2}
	"ATTRIB_ESPIONAGE_BONUS": (1, 0x0001b2b6, 0x2, False), ## {0x0 = +0, 0x1 = +1, 0x2 = +2}
	"ATTRIB_LOW_G_WORLD":     (1, 0x0001b2b7, 0x0, False), ## does not change homeworld after game-start
	"ATTRIB_HIGH_G_WORLD":    (1, 0x0001b2b8, 0x1, False), ## does not change homeworld after game-start
	"ATTRIB_AQUATIC":         (1, 0x0001b2b9, 0x0, False),
	"ATTRIB_SUBTERRANEAN":    (1, 0x0001b2ba, 0x1, False),
	"ATTRIB_POOR_WORLD":      (1, 0x0001b2bb, 0x1,  True), ## does not change homeworld after game-start
	"ATTRIB_RICH_WORLD":      (1, 0x0001b2bc, 0x1,  True), ## does not change homeworld after game-start
	"ATTRIB_ARTIFACT_WORLD":  (1, 0x0001b2bd, 0x1,  True), ## does not change homeworld after game-start

	"ATTRIB_CYBERNETIC":      (1, 0x0001b2be, 0x0, False),
	"ATTRIB_LITHOVORE":       (1, 0x0001b2bf, 0x1, False),
	"ATTRIB_REPULSIVE":       (1, 0x0001b2c0, 0x0, False),
	"ATTRIB_CHARISMATIC":     (1, 0x0001b2c1, 0x1, False),
	"ATTRIB_UNCREATIVE":      (1, 0x0001b2c2, 0x0, False),
	"ATTRIB_CREATIVE":        (1, 0x0001b2c3, 0x0, False),
	"ATTRIB_TOLERANT":        (1, 0x0001b2c4, 0x1, False),
	"ATTRIB_EXPERT_TRADERS":  (1, 0x0001b2c5, 0x1, False),
	"ATTRIB_TELEPATHIC":      (1, 0x0001b2c6, 0x1, False),
	"ATTRIB_LUCKY":           (1, 0x0001b2c7, 0x1, False),
	"ATTRIB_OMNISCIENT":      (1, 0x0001b2c8, 0x0, False),
	"ATTRIB_STEALTH_SHIPS":   (1, 0x0001b2c9, 0x1, False),
	"ATTRIB_TRANS_DIMENS":    (1, 0x0001b2ca, 0x1, False),
	"ATTRIB_WARLORD":         (1, 0x0001b2cb, 0x1, False),
}



## includes player's race
##
## player is always race 0, race i is predetermined (so i=1
## is NOT the first race encountered except by coincidence)
## during game initialization --> we can not know a-priori
## which race <i> maps to
##
## relation states seem to be maintained bidirectionally
## (hopefully in fixed-size arrays so offsets do not vary
## with number of players)
##
## if stance-state is altered before first contact, meeting
## that race will reset it to the default (but dispositions
## are kept)
##
## since any race can interact with any other race, model
## their relations using 2D matrices (however note that we
## only care about the ones to or from race 0)
##
MAX_NUM_RACES = 8
## sizeof(struct t_player); PLAYER_RECORD_SIZE in OpenMOO2
PLAYER_STRUCT_SIZE = 3753

## addresses to patch (filled in later)
RACE_DISPOS_OFFSET_MATRIX = [ ([             0x0 ] * MAX_NUM_RACES) for x in xrange(MAX_NUM_RACES) ]
RACE_XXXXXX_OFFSET_MATRIX = [ ([             0x0 ] * MAX_NUM_RACES) for x in xrange(MAX_NUM_RACES) ]
RACE_STANCE_OFFSET_MATRIX = [ ([             0x0 ] * MAX_NUM_RACES) for x in xrange(MAX_NUM_RACES) ]
RACE_AMBASS_OFFSET_MATRIX = [ ([(0x0,0x0,0x0,0x0)] * MAX_NUM_RACES) for x in xrange(MAX_NUM_RACES) ]
## (trade, research)-treaty flag addresses
RACE_TREATY_OFFSET_MATRIX = [ ([       (0x0, 0x0)] * MAX_NUM_RACES) for x in xrange(MAX_NUM_RACES) ]
RACE_TRIBUT_OFFSET_MATRIX = [ ([             0x0 ] * MAX_NUM_RACES) for x in xrange(MAX_NUM_RACES) ]

## values to patch in
RACE_DISPOS_VALUE_MATRIX = [ ([RACE_DISPOS_TYPES["DISPOS_NEUTRAL"]] * MAX_NUM_RACES) for x in xrange(MAX_NUM_RACES) ]
RACE_XXXXXX_VALUE_MATRIX = [ ([                               0x0 ] * MAX_NUM_RACES) for x in xrange(MAX_NUM_RACES) ]
RACE_STANCE_VALUE_MATRIX = [ ([RACE_STANCE_TYPES["STANCE_DEFAULT"]] * MAX_NUM_RACES) for x in xrange(MAX_NUM_RACES) ]
RACE_AMBASS_VALUE_MATRIX = [ ([               (0x0, 0x0, 0x0, 0x0)] * MAX_NUM_RACES) for x in xrange(MAX_NUM_RACES) ]
RACE_TREATY_VALUE_MATRIX = [ ([                               0x0 ] * MAX_NUM_RACES) for x in xrange(MAX_NUM_RACES) ]
RACE_TRIBUT_VALUE_MATRIX = [ ([                               0x0 ] * MAX_NUM_RACES) for x in xrange(MAX_NUM_RACES) ]

## how friendly we want AI's to be initially
RACE_SET_DISPOS_MATRIX = [ ([ False        ] * MAX_NUM_RACES) for x in xrange(MAX_NUM_RACES)]
RACE_SET_STANCE_MATRIX = [ ([ False        ] * MAX_NUM_RACES) for x in xrange(MAX_NUM_RACES)]
RACE_SET_AMBASS_MATRIX = [ ([ False        ] * MAX_NUM_RACES) for x in xrange(MAX_NUM_RACES)]
RACE_SET_TREATY_MATRIX = [ ([(False, False)] * MAX_NUM_RACES) for x in xrange(MAX_NUM_RACES)]
RACE_SET_TRIBUT_MATRIX = [ ([ False        ] * MAX_NUM_RACES) for x in xrange(MAX_NUM_RACES)]



for i in xrange(1, MAX_NUM_RACES):
	## magic offsets correspond to the third race (index 2)
	## and we want to start counting at the second (index 1)
	j = i - 1
	## ditto but with different offset again
	k = i - (MAX_NUM_RACES >> 1) + 1

	## race 0 (us) to race i
	RACE_DISPOS_OFFSET_MATRIX[0][i] = 0x0001b026 + j*1
	RACE_XXXXXX_OFFSET_MATRIX[0][i] = 0x0001b02e + j*1
	RACE_STANCE_OFFSET_MATRIX[0][i] = 0x0001b036 + j*1
	RACE_AMBASS_OFFSET_MATRIX[0][i] = (0x0001b09f + j*2, 0x0001b0af + j*2, 0x0001b0bf + j*2, 0x0001b0cf + j*2)

	## race i to race 0
	RACE_DISPOS_OFFSET_MATRIX[i][0] = 0x0001bece + (j * PLAYER_STRUCT_SIZE)
	RACE_XXXXXX_OFFSET_MATRIX[i][0] = 0x0001bed6 + (j * PLAYER_STRUCT_SIZE)
	RACE_STANCE_OFFSET_MATRIX[i][0] = 0x0001bede + (j * PLAYER_STRUCT_SIZE)
	RACE_AMBASS_OFFSET_MATRIX[i][0] = (0x0001bf46 + (j * PLAYER_STRUCT_SIZE), 0x0001bf56 + (j * PLAYER_STRUCT_SIZE), 0x0001bf66 + (j * PLAYER_STRUCT_SIZE), 0x0001bf76 + (j * PLAYER_STRUCT_SIZE))

	## race 0 (us) to race i
	## 0x0001b040 + k + MAX_NUM_RACES*2 --> "giving 10% tribute"
	## 0x0001b04d + k + MAX_NUM_RACES --> ?
	RACE_TREATY_OFFSET_MATRIX[0][i] = (0x0001b040 + k, 0x0001b040 + k + MAX_NUM_RACES)
	## race i to race 0
	RACE_TREATY_OFFSET_MATRIX[i][0] = ((0x0001dc38 + 0) + k * PLAYER_STRUCT_SIZE, (0x0001dc38 + 2) + k * PLAYER_STRUCT_SIZE)
	RACE_TRIBUT_OFFSET_MATRIX[i][0] = 0x000216ec - (PLAYER_STRUCT_SIZE * j)

for i in xrange(1, MAX_NUM_RACES):
	## race 0 (us) to race i (meaning of the 4-tuple values is unknown)
	RACE_DISPOS_VALUE_MATRIX[0][i] = RACE_DISPOS_TYPES["DISPOS_UNITY"]
	RACE_STANCE_VALUE_MATRIX[0][i] = RACE_STANCE_TYPES["STANCE_NAGGPACT"]
	RACE_AMBASS_VALUE_MATRIX[0][i] = (0x0, 0x0, 0x0, 0x0)
	RACE_TREATY_VALUE_MATRIX[0][i] = (0x1, 0x1)

	RACE_SET_DISPOS_MATRIX[0][i] = True
	RACE_SET_STANCE_MATRIX[0][i] = True
	RACE_SET_AMBASS_MATRIX[0][i] = True
	RACE_SET_TREATY_MATRIX[0][i] = (True, True)

	## race i to race 0 (meaning of the 4-tuple values is unknown)
	RACE_DISPOS_VALUE_MATRIX[i][0] = RACE_DISPOS_TYPES["DISPOS_UNITY"]
	RACE_STANCE_VALUE_MATRIX[i][0] = RACE_STANCE_TYPES["STANCE_NAGGPACT"]
	RACE_AMBASS_VALUE_MATRIX[i][0] = (0x0, 0x0, 0x0, 0x0)
	RACE_TREATY_VALUE_MATRIX[i][0] = (0x1, 0x1)
	RACE_TRIBUT_VALUE_MATRIX[i][0] = RACE_TRIBUT_TYPES["MAX_PERCENT_TRIBUTE"]

	RACE_SET_DISPOS_MATRIX[i][0] = True
	RACE_SET_STANCE_MATRIX[i][0] = True
	RACE_SET_AMBASS_MATRIX[i][0] = True
	RACE_SET_TREATY_MATRIX[i][0] = (True, True)
	RACE_SET_TRIBUT_MATRIX[i][0] = True



def read_file(name):
	f = open(name, 'rb')
	s = f.read()
	b = [ord(s[i]) for i in xrange(len(s))]
	f = f.close()
	return b

def write_file(data, name):
	f = open(name, 'wb')
	for i in xrange(len(data)):
		f.write("%c" % data[i])
	f.close()
	print("[write_file] wrote %s" % name)

## NOTE: these assume little-endian byte order (x86)
##   bytes[i+0] is byte 0 (LSB)
##   bytes[i+1] is byte 1
##   bytes[i+2] is byte 2
##   bytes[i+3] is byte 3 (MSB)
##
def read_int32(bytes, index):
	value  = (bytes[index + 0] <<  0)
	value += (bytes[index + 1] <<  8)
	value += (bytes[index + 2] << 16)
	value += (bytes[index + 3] << 24)
	return value
def read_int16(bytes, index):
	value  = (bytes[index + 0] <<  0)
	value += (bytes[index + 1] <<  8)
	return value
def read_int8(bytes, index):
	return ((bytes[index + 0]) << 0)
def read_int8s(bytes, index, length):
	str_chars = ""
	for i in xrange(length):
		str_chars += chr(bytes[index + i])
	## return (bytes[index: index + length])
	return str_chars

def read_int(bytes, length, index):
	if (length == 1): return (read_int8(bytes, index))
	if (length == 2): return (read_int16(bytes, index))
	if (length == 4): return (read_int32(bytes, index))
	return 0


def edit_int32(bytes, index, value):
	bytes[index + 0] = (value >>  0) & 0xff
	bytes[index + 1] = (value >>  8) & 0xff
	bytes[index + 2] = (value >> 16) & 0xff
	bytes[index + 3] = (value >> 24) & 0xff
def edit_int16(bytes, index, value):
	bytes[index + 0] = (value >>  0) & 0xff
	bytes[index + 1] = (value >>  8) & 0xff
def edit_int8(bytes, index, value):
	bytes[index + 0] = (value >> 0) & 0xff
def edit_int(bytes, length, index, value):
	if (length == 1): edit_int8(bytes, index, value)
	if (length == 2): edit_int16(bytes, index, value)
	if (length == 4): edit_int32(bytes, index, value)



def diff_files(save_file_a, save_file_b):
	if (not os.path.isfile(save_file_a)):
		print("[diff_file] file \"%s\" does not exist" % save_file_a)
		return -1
	if (not os.path.isfile(save_file_b)):
		print("[diff_file] file \"%s\" does not exist" % save_file_b)
		return -1

	inp_bytes = [read_file(f) for f in [save_file_a, save_file_b]]

	assert(len(inp_bytes) == 2)
	assert(len(inp_bytes[0]) == len(inp_bytes[1]))

	last_diff_addr = 0x0
	num_diff_bytes = 0

	for i in xrange(len(inp_bytes[0])):
		b0 = inp_bytes[0][i]
		b1 = inp_bytes[1][i]

		if (b0 == b1):
			continue

		## insert spacing for int24-sized and larger jumps between diffs 
		if ((i - last_diff_addr) >= 0x3):
			print("")

		print("[n=%04d][addr=0x%08x::diff=0x%08x] {b0,b1}={0x%02x (%3d), 0x%02x (%3d)}" % (num_diff_bytes, i, i - last_diff_addr, b0, b0, b1, b1))

		last_diff_addr = i
		num_diff_bytes += 1

	return 0


def edit_file(save_file):
	if (not os.path.isfile(save_file)):
		print("[edit_file] file \"%s\" does not exist" % save_file)
		return -1

	inp_bytes = read_file(save_file)
	out_bytes = inp_bytes[:]
	num_turns = read_int(inp_bytes, PLAYER_DATA["STARDATE"][0], PLAYER_DATA["STARDATE"][1]) - 35000

	## name of a saved game starts at offset 0x9, string is 0x15
	## bytes long at least (29=0x1D can be entered in save-menu)
	print("[edit_file]")
	print("\tMAGICNUM=%d" % read_int32(inp_bytes, 0x0))
	print("\tSAVENAME=%s" % read_int8s(inp_bytes, 0x9, 20))
	print("\tSTARDATE=%d (TURN=%d)" % (num_turns + 35000, num_turns))
	print("")
	print("\tNUM_BCS=%d" % read_int(inp_bytes, PLAYER_DATA["NUM_BCS"][0], PLAYER_DATA["NUM_BCS"][1]))
	print("\tNUM_RPS=%d" % read_int(inp_bytes, PLAYER_DATA["NUM_RPS"][0], PLAYER_DATA["NUM_RPS"][1]))

	## edit_int8(out_bytes, 0x00000043, 0xb)
	##
	## if editing just these two, colony-ship is still buyable (244prod) --> not true
	## edit_int8(out_bytes, 0x00000a8f, 0xfe)
	## edit_int8(out_bytes, 0x00000a90, 0x7)
	##
	## related to amount of BC's generated for us from trade-treaties (0xfffc - 65536 = -4 = 0x4)
	##
	## edit_int8(out_bytes, 0x0001afbc, 0xfc)
	## edit_int8(out_bytes, 0x0001afbd, 0xff)
	## edit_int8(out_bytes, 0x0001afcc, 0x04)
	## edit_int8(out_bytes, 0x0001f90f, 0x04)
	if (EDIT_PLAYER_STATS):
		for key in PLAYER_DATA:
			if (PLAYER_DATA[key][3]):
				continue

			edit_int(out_bytes, PLAYER_DATA[key][0], PLAYER_DATA[key][1], PLAYER_DATA[key][2])

	if (EDIT_AI_RELATIONS):
		for i in xrange(MAX_NUM_RACES):
			## us to them
			if (RACE_SET_DISPOS_MATRIX[0][i]):
				## note: the relation needs to be updated bidirectionally or changes
				## will not persist across turn, ambassador communications remain in
				## "angry" state for a few turns afterwards
				edit_int8(out_bytes, RACE_DISPOS_OFFSET_MATRIX[0][i], RACE_DISPOS_VALUE_MATRIX[0][i])
			if (RACE_SET_STANCE_MATRIX[0][i]):
				edit_int8(out_bytes, RACE_STANCE_OFFSET_MATRIX[0][i], RACE_STANCE_VALUE_MATRIX[0][i])
			if (RACE_SET_AMBASS_MATRIX[0][i]):
				## these four have something to do with ambassadorial communication
				## with race i (all zero's means the ambassador is always available)
				##
				## if first three are 0x0000, fourth seems to matter
				## if first three are 0xff2e, fourth seems not to matter
				##
				## right after first contact, all four values are 0
				## could these control receptiveness to proposals?
				## or do they act like a timer or a random number?
				edit_int16(out_bytes, RACE_AMBASS_OFFSET_MATRIX[0][i][0], RACE_AMBASS_VALUE_MATRIX[0][i][0])
				edit_int16(out_bytes, RACE_AMBASS_OFFSET_MATRIX[0][i][1], RACE_AMBASS_VALUE_MATRIX[0][i][1])
				edit_int16(out_bytes, RACE_AMBASS_OFFSET_MATRIX[0][i][2], RACE_AMBASS_VALUE_MATRIX[0][i][2])
				edit_int16(out_bytes, RACE_AMBASS_OFFSET_MATRIX[0][i][3], RACE_AMBASS_VALUE_MATRIX[0][i][3])

			## them to us
			if (RACE_SET_DISPOS_MATRIX[i][0]):
				edit_int8(out_bytes, RACE_DISPOS_OFFSET_MATRIX[i][0], RACE_DISPOS_VALUE_MATRIX[i][0])
			if (RACE_SET_STANCE_MATRIX[i][0]):
				edit_int8(out_bytes, RACE_STANCE_OFFSET_MATRIX[i][0], RACE_STANCE_VALUE_MATRIX[i][0])
			if (RACE_SET_AMBASS_MATRIX[i][0]):
				edit_int16(out_bytes, RACE_AMBASS_OFFSET_MATRIX[i][0][0], RACE_AMBASS_VALUE_MATRIX[i][0][0])
				edit_int16(out_bytes, RACE_AMBASS_OFFSET_MATRIX[i][0][1], RACE_AMBASS_VALUE_MATRIX[i][0][1])
				edit_int16(out_bytes, RACE_AMBASS_OFFSET_MATRIX[i][0][2], RACE_AMBASS_VALUE_MATRIX[i][0][2])
				edit_int16(out_bytes, RACE_AMBASS_OFFSET_MATRIX[i][0][3], RACE_AMBASS_VALUE_MATRIX[i][0][3])


			## us to them
			if (RACE_SET_TREATY_MATRIX[0][i][0]):
				edit_int8(out_bytes, RACE_TREATY_OFFSET_MATRIX[0][i][0], RACE_TREATY_VALUE_MATRIX[0][i][0])
			if (RACE_SET_TREATY_MATRIX[0][i][1]):
				edit_int8(out_bytes, RACE_TREATY_OFFSET_MATRIX[0][i][1], RACE_TREATY_VALUE_MATRIX[0][i][1])

			## them to us
			if (RACE_SET_TREATY_MATRIX[i][0][0]):
				edit_int8(out_bytes, RACE_TREATY_OFFSET_MATRIX[i][0][0], RACE_TREATY_VALUE_MATRIX[i][0][0])
			if (RACE_SET_TREATY_MATRIX[i][0][1]):
				edit_int8(out_bytes, RACE_TREATY_OFFSET_MATRIX[i][0][1], RACE_TREATY_VALUE_MATRIX[i][0][1])
			if (RACE_SET_TRIBUT_MATRIX[i][0]):
				edit_int8(out_bytes, RACE_TRIBUT_OFFSET_MATRIX[i][0], RACE_TRIBUT_VALUE_MATRIX[i][0])


	write_file(out_bytes, DEF_SAVE_NAME)
	return 0



def read_hof(hof_file):
	## HOF file-format is structure-of-arrays (not array-of-structures)
	## scores are stored contiguously, so are player and race names and
	## difficulty levels
	##
	## NOTE:
	##   names are in ascending order of score, except for the default
	##   HOF which appears to be randomly generated whenever the file
	##   is deleted and stores them in descending order
	##
	##    1.        Moise      Human       Tutor  1550 (0x060e)
	##    2.         Irma      Human        Easy  1470 (0x05be)
	##    3.        J W R  Trilarian     Average  1295 (0x050f)
	##    4.       Cereal     Meklar        Hard  1230 (0x04ce)
	##    5.      Cadfael     Darlok  Impossible  1095 (0x0447)
	##    6. Ripping_Fang     Human        Tutor   980 (0x03d4)
	##    7.                                       840 (0x0348)
	##    8.                                       655 (0x028f)
	##    9.                                       540 (0x021c)
	##   10.        Chewy    Mrrshan  Impossible   400 (0x0190)
	##
	hof_bytes  = read_file(hof_file)
	hof_size   = 10
	str_size   = 20
	name_addr  = 0x02 ## int8[str_size]
	race_addr  = 0xfc ## int8[str_size]
	score_addr = 0xca ## int16
	level_addr = 0xf2 ## int8
	level_strs = ["Tutor", "Easy", "Average", "Hard", "Impossible"]

	print("[RANK][    PLAYER NAME][    PLAYER RACE][DIFFICULTY][SCORE]")
	for i in xrange(hof_size):
		player_name  = read_int8s(hof_bytes, name_addr + (i * str_size), str_size)
		player_race  = read_int8s(hof_bytes, race_addr + (i * str_size), str_size)
		player_name  = player_name[0: player_name.find('\x00')] ## ignore trailing garbage
		player_race  = player_race[0: player_race.find('\x00')] ## ignore trailing garbage
		player_score = read_int16(hof_bytes, score_addr + (i * 2))
		player_level = read_int8 (hof_bytes, level_addr + (i * 1))
		print("[%4d][%15s][%15s][%10s][%5d]" % (i + 1, player_name, player_race, level_strs[player_level], player_score))

def edit_hof(hof_file):
	hof_size   = 10
	score_addr = 0xca

	b = read_file(hof_file)
	f = open(hof_file, 'wb')

	## wipe everyone's score
	for i in xrange(hof_size):
		edit_int16(b, score_addr + (i * 2), 0)

	write_file(b, HOF_FILE_NAME)

def check_binary_hash(bin_file):
	f = open(bin_file, 'r')
	h = hashlib.sha256(f.read())
	f = f.close()
	return (h.hexdigest() == BIN_FILE_HASH)



def main(argc, argv):
	## verify checksum of the binary if it exists
	if (os.path.isfile(BIN_FILE_NAME)):
		if (not check_binary_hash(BIN_FILE_NAME)):
			print("[main] checksum of %s does not match expected value" % (BIN_FILE_NAME))
			return -1

	## dump the hall-of-fame if it exists
	if (os.path.isfile(HOF_FILE_NAME)):
		read_hof(HOF_FILE_NAME)

	if (argc == 2):
		return (edit_file(argv[1]))
	if (argc == 3):
		return (diff_files(argv[1], argv[2]))

	print("[main] usage: %s <edit_save> | [<diff_save_a>, <diff_save_b>]" % (argv[0]))
	return 0

sys.exit(main(len(sys.argv), sys.argv))

