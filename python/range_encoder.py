import sys

## Range coding: conceptually encodes all the symbols of the message into one number
## Huffman coding: assigns each symbol a bit-pattern and concatenates all bit-patterns
##
## given a large-enough range of integers, and a probability estimation for the symbols,
## the initial range can easily be divided into sub-ranges whose sizes are proportional
## to the probability of the symbol they represent
##
## each symbol of the message can then be encoded in turn, by reducing the current range
## down to just that sub-range which corresponds to the next symbol to be encoded (note
## that the initial interval should have sufficient precision to distinguish between all
## of the symbols in the message alphabet)

def BuildSymbolTable(messageString):
	symbolTable = {}
	symbolOrder = [''] * len(messageString)

	symbolIdx = 0
	## cumulative probability
	symbolSum = 0.0

	for i in xrange(len(messageString)):
		symbol = messageString[i]

		if (symbol in symbolTable):
			symbolTable[symbol][1] += 1
			continue

		## new symbol entry (index, count, prob, min-prob, max-prob)
		symbolTable[symbol] = [symbolIdx, 1,  0.0, 0.0, 0.0]
		symbolOrder[symbolIdx] = symbol
		symbolIdx += 1

	## set relative frequencies (probability estimates)
	for i in xrange(len(messageString)):
		symbol = messageString[i]
		symbolTable[symbol][2] = symbolTable[symbol][1] * (1.0 / len(messageString))

	## processing symbols in an order different from their message ordering
	## should not matter for the per-symbol probability masses, but the range
	## code in EncodeSymbol is sensitive to it
	for symbolIdx in xrange(len(symbolOrder)):
		symbol = symbolOrder[symbolIdx]

		if (symbol == ''):
			break

		## note: these are not true probabilities but probability intervals
		## eg. 'A' maps to (0.0, 0.6), 'B' to (0.6, 0.8), '0' to (0.8, 1.0)
		## the true probability of a symbol is given by (max - min)
		symbolTable[symbol][3] = symbolSum
		symbolTable[symbol][4] = symbolSum + symbolTable[symbol][2]

		symbolSum += symbolTable[symbol][2]

	return symbolTable

def EncodeSymbol(symbol, symbolTable, symbolRange):
	minSymRange = symbolRange[0]
	maxSymRange = symbolRange[1]
	relSymRange = maxSymRange - minSymRange

	minSymProb = symbolTable[symbol][3]
	maxSymProb = symbolTable[symbol][4]

	## introduces less rounding error than "minSymRange + int(...)"
	minSymRange = int(minSymRange + relSymRange * (minSymProb             ))
	maxSymRange = int(minSymRange + relSymRange * (maxSymProb - minSymProb))

	assert(maxSymRange > minSymRange)

	symbolRange[0] = minSymRange
	symbolRange[1] = maxSymRange

def EncodeMessage(messageString, symbolTable):
	codingBase = max(10, len(symbolTable.keys()))
	symbolRange = [0, codingBase ** len(messageString)]

	for symbol in messageString:
		EncodeSymbol(symbol, symbolTable, symbolRange)

	## the original message can be represented by any number
	## within the final range (eg. [25056, 25920] which lets
	## us pick eight 3-digit prefixes: 251*, 252*, ..., 259*)
	## so just choose the mid-way point
	##
	messageCode = int((symbolRange[0] + symbolRange[1]) * 0.5)

	print("[EncMsg] %s --> %d" % (messageString, messageCode))
	return messageCode

def DecodeSymbol(symbolRange, messageCode, symbolTable):
	minSymRange = symbolRange[0]
	maxSymRange = symbolRange[1]
	relSymRange = symbolRange[1] - symbolRange[0]

	symbolProb = (messageCode - minSymRange) * (1.0 / relSymRange)
	nextSymbol = ''

	## find the symbol whose probability interval contains symbolProb
	for symbol in symbolTable:
		minSymProb = symbolTable[symbol][3]
		maxSymProb = symbolTable[symbol][4]

		## mins are inclusive, maxs are exclusive
		if (minSymProb <= symbolProb and symbolProb < maxSymProb):
			minSymRange = int(minSymRange + relSymRange * (             minSymProb))
			maxSymRange = int(minSymRange + relSymRange * (maxSymProb - minSymProb))
			nextSymbol = symbol
			break

	symbolRange[0] = minSymRange
	symbolRange[1] = maxSymRange

	assert(nextSymbol != '')
	return nextSymbol

def DecodeMessage(messageCode, messageSize, symbolTable):
	messageString = ""

	codingBase = max(10, len(symbolTable.keys()))
	symbolRange = [0, codingBase ** messageSize]

	for i in xrange(messageSize):
		messageString += DecodeSymbol(symbolRange, messageCode, symbolTable)

	print("[DecMsg] %d --> %s" % (messageCode, messageString))
	return messageString

def main(argc, argv):
	if (argc > 1):
		rawMessage = argv[1]
	else:
		rawMessage = "AABA0"

	symbolTable = BuildSymbolTable(rawMessage)
	encMessage = EncodeMessage(rawMessage, symbolTable)
	decMessage = DecodeMessage(encMessage, len(rawMessage), symbolTable)

	assert(decMessage == rawMessage)
	return 0

sys.exit(main(len(sys.argv), sys.argv))

