# coding=utf-8

"""
RCG rules have the general schema f(x_1, ..., x_n) --> α
where α is either the empty string or a string of predicates

The arguments x_i consist of strings of terminal symbols and / or
variable symbols, which pattern-match against actual argument values


Adjacent variables constitute a family of matches against partitions,
so that the argument xy, with two variables, matches the literal string
ab in three different ways:
   1. x = eps, y = ab
   2. x =  a,  y =  b
   3. x =  ab, y = eps

Predicate terms come in two forms, positive (which produce the empty
string on success), and negative (which produce the empty string on
failure / if the positive term does not produce the empty string).
Negative terms are denoted the same as positive terms, with an overbar
    ----------------
    f(x_1, ..., x_n)


The rewrite semantics for RCGs is simple. Given a predicate string
(α_1,...,α_n), where the symbols α_i are terminal strings, then if
there is a rule f(x_1, ..., x_n) --> β in the grammar that the
predicate string matches, the predicate string is replaced by β,
substituting for the matched variables in each x_i.

For example, given the rule f(x, ayb) --> g(axb, y), where x and y are
variable symbols and a and b are terminal symbols, the predicate string
f(a, abb) can be rewritten as g(aab, b) because f(a, abb) matches f(x, ayb)
when x = a, y = b.

Similarly, if there were a rule f(x, ayb) --> f(x, x) f(y, y) then
f(a, abb) could be rewritten as f(a, a) f(b, b).


A proof / recognition of a string α is done by showing that S(α)
produces the empty string. For the individual rewrite steps, when
multiple alternative variable matches are possible, any rewrite
which could lead the whole proof to succeed is considered. Thus,
if there is at least one way to produce the empty string from the
initial string S(α), the proof is considered a success, regardless
of how many other ways to fail exist.
##

example
	grammar rules:
		1. S(xyz) --> A(x, y, z)
		2. A(ax, ay, az) --> A(x, y, z)   strips off the non-variable 'a'
		3. A(bx, by, bz) --> A(x, y, z)   strips off the non-variable 'b'
		4. A(eps, eps, eps) --> eps

	parsing of abbabbabb with this grammar
		S(abbabbabb) --> A(abb, abb, abb)   [rule 1: x=abb, y=abb, z=abb]
		A(abb, abb, abb) --> A(bb, bb, bb)  [rule 2: x= bb, y= bb, z= bb]
		A(bb, bb, bb) --> A(b, b, b)        [rule 3: x=  b, y=  b, z=  b]
		A(b, b, b) --> A(eps, eps, eps)     [rule 3: x, y, z are instantiated to eps]
		A(eps, eps, eps) --> eps            [rule 4]
"""




from copy import deepcopy

def PrettyPrint(T):
	if (type(T) == type([])):
		for e in T:
			PrettyPrint(e)

	elif (type(T) == type({})):
		## sort keys according to their natural ordering
		K = T.keys()
		K.sort()
		L = []

		for k in K:
			L += [(k, T[k])]

		print("%s" % L)

	else:
		print("%s" % T)

def Match(string, pattern):
	bindings = {}
	solutions = []

	## initialize variable-bindings
	## to empty ("epsilon") strings
	for v in pattern:
		bindings[v] = ("", -1, -1, -1)

	MatchRec(string, 0,  pattern, 0,  bindings, 0, solutions)
	return solutions

def MatchRec(string, stringIdx, pattern, patternIdx, bindings, depth, solutions):
	for i in xrange(patternIdx, len(pattern)):
		variable = pattern[i]

		## NOTE the " + 1" upper bound in the inner loop
		## this is needed because the span from <j< to <k>
		## must cover all substrings of <string>, including
		## <string> itself (the second argument to xrange()
		## is exclusive)
		for j in xrange(stringIdx, len(string)):
			for k in xrange(j + 1, len(string) + 1):
				binding = ""

				if (bindings[variable][0] == ""):
					binding = string[j: k]
					bindings[variable] = (binding, i, j, k)
				else:
					## if this variable already has a binding, we
					## have encountered it earlier in the pattern
					## (ie. it occurs multiple times)
					binding = bindings[variable][0]

					if (string[j: k] != binding):
						return False

				assert((k - j) == len(binding))
				assert(k == (j + len(binding)))

				if (not MatchRec(string, k,  pattern, i + 1,  bindings, depth + 1, solutions)):
					## undo the created binding if somewhere deeper
					## in the recursive trace we found a mis-match
					bindings[variable] = ("", -1, -1, -1)

	sstring = ""

	## NOTE: dictionary ordering means we need to
	## iterate over variables in order they appear
	## in <pattern>
	for variable in pattern:
		sstring += bindings[variable][0]

	if (sstring == string):
		## pattern matched against string
		solutions += [deepcopy(bindings)]

	## if we return true at this point, only one
	## matching combination out of all potential
	## variable bindings is generated
	## return (sstring == string)
	return False



PrettyPrint(Match("abba",   "x"))
print("%s" % "---------")

PrettyPrint(Match(  "ab",  "xy"))
print("%s" % "---------")

## NOTE: produces duplicate solutions (like other string/pattern combinations)
##    1) 'x' at pattern[0] gets bound to 'a' at string[0], then 'y' to 'bb'
##    2) 'x' at pattern[0] gets bound to 'a' at string[3], then 'y' to 'bb'
PrettyPrint(Match("abba", "xyx"))
print("%s" % "---------")

PrettyPrint(Match("abba", "xyy"))
print("%s" % "---------")

PrettyPrint(Match("abba", "xxy"))
print("%s" % "---------")

PrettyPrint(Match( "abb", "xyx"))
print("%s" % "---------")

PrettyPrint(Match( "abb", "xyy"))
print("%s" % "---------")

PrettyPrint(Match( "abab", "xy"))
print("%s" % "---------")

PrettyPrint(Match( "abab", "xxx"))
print("%s" % "---------")

PrettyPrint(Match( "abc", "xyz"))
print("%s" % "---------")

PrettyPrint(Match("abbcddeffa", "xyzx"))
print("%s" % "---------")

PrettyPrint(Match( "a", "xyz"))
print("%s" % "---------")

