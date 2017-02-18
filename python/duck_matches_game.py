## "Duck" gameplay rules:
##   start with a pile of matches of some size, two players
##   each player removes 1, 2, or 3 matches from pile in turn
##   if pile is empty, winner is player whose move cleared it
##
## assumptions:
##   player 0 always moves first, initial pile always contains
##   an *odd* number of matches so no 50-50 split is possible
##
def Play(curPlayer, legalMoves, winStats, pileSize, matches, depth):
	if (pileSize == 0):
		## empty pile, find the winning player (0 or 1)
		w = 1 - (matches[curPlayer] & 1)

		winStats[w] += 1
		winStats[2] += 1

		return w

	for move in legalMoves:
		if (pileSize >= move):
			## if player is 0, opponent is 1 and v.v.
			nxtPlayer = 1 - curPlayer

			## add taken matches to players's pile
			matches[curPlayer] += move
			matches[nxtPlayer] += 0

			Play(nxtPlayer, legalMoves, winStats, pileSize - move, matches, depth + 1)

			matches[curPlayer] -= move
			matches[nxtPlayer] -= 0

def DuckGame(maxPileSize, legalMoves):
	## number of matches taken from pile per player
	numMatches = [0, 0]
	## games won by A, games won by B, total
	gameStats = [0, 0, 0]

	## generate all odd numbers from 1 to maxPileSize inclusive
	for pileSize in range(1, maxPileSize + 2, 2):
		Play(0, legalMoves, gameStats, pileSize, numMatches, 0)

		print("[DuckGame] pileSize=%d gameStats=%s" % (pileSize, gameStats))

		numMatches[0] = 0
		numMatches[1] = 0

		gameStats[0] = 0
		gameStats[1] = 0
		gameStats[2] = 0

DuckGame(15, [3, 2, 1])

