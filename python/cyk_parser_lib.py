from math import log

class cyk_parse_tree:
	def __init__(self, parent, symbol):
		self.parent   = parent
		self.children = []
		self.symbol   = symbol
		self.logprob  = 0.0

	def Delete(self):
		for c in self.children:
			c.Delete()
		del self


	def print_tree(self, tree_string):
		print("[cyk_parse_tree::print] parsed representation of \"%s\"" % (tree_string))
		self.print_tree_rec(self, 0)

	def print_tree_rec(self, node, depth):
		if (len(node.children) > 0):
			rule_lhs = node.symbol
			rule_rhs = ""

			for c in node.children:
				rule_rhs += (c.symbol + " ")

			print("%s%s ==> %s" % (('\t' * (depth + 1)), rule_lhs, rule_rhs))

		for c in node.children:
			self.print_tree_rec(c, depth + 1)


	def insert_rule(self, rule_counts, lhs_counts, rule_lhs, rule_rhs):
		if (rule_counts.has_key(rule_lhs)):
			if (rule_counts[rule_lhs].has_key(rule_rhs)):
				rule_counts[rule_lhs][rule_rhs] += 1
			else:
				rule_counts[rule_lhs][rule_rhs] = 1
		else:
			rule_counts[rule_lhs] = {rule_rhs: 1}

		if (lhs_counts.has_key(rule_lhs)):
			lhs_counts[rule_lhs] += 1
		else:
			lhs_counts[rule_lhs] = 1


	def extract_rules(self, glob_rule_cnts, glob_lhs_cnts):
		tree_rule_cnts = {}
		tree_lhs_cnts  = {}

		self.extract_rules_rec(self, tree_rule_cnts, tree_lhs_cnts, glob_rule_cnts, glob_lhs_cnts)

		del tree_rule_cnts
		del tree_lhs_cnts

	def extract_rules_rec(self, node, tree_rule_cnts, tree_lhs_cnts, glob_rule_cnts, glob_lhs_cnts):
		numChildren = len(node.children)

		if (numChildren > 0):
			## each tree should be properly binarized
			## (ie. all rules are of the form A ==> BC
			## or A ==> a, except for TOP ==> S)
			if (numChildren == 1):
				b0 = (len((node.children[0]).children) == 0)
				b1 = (node.symbol == "CAT_TOP")
				assert(b0 or b1)
			else:
				assert(numChildren == 2)

			rule_lhs = node.symbol
			rule_rhs = [""] * len(node.children)

			for i in xrange(len(node.children)):
				rule_rhs[i] = (node.children[i]).symbol

			rule_rhs = tuple(rule_rhs)

			self.insert_rule(tree_rule_cnts, tree_lhs_cnts, rule_lhs, rule_rhs)
			self.insert_rule(glob_rule_cnts, glob_lhs_cnts, rule_lhs, rule_rhs)

			for c in node.children:
				self.extract_rules_rec(c, tree_rule_cnts, tree_lhs_cnts, glob_rule_cnts, glob_lhs_cnts)


	## convert the tree representation of a string (read from the
	## training data) in bracket-notation to Chomsky Normal Form
	#
	## NOTE: starts the conversion _below_ the "TOP" node
	def convert_to_cnf(self):
		self.convert_to_cnf_rec(self.children[0])

	def convert_to_cnf_rec(self, A):
		while (len(A.children) == 1):
			B = A.children[0]

			if (len(B.children) > 0):
				del A.children; A.children = B.children
				for child in A.children: child.parent = A
			else:
				break

		for c in A.children:
			self.convert_to_cnf_rec(c)


	## convert the representation of a CNF-tree (assigned
	## by the parser to a sentence read from the test-data)
	## to a string in bracket-notation
	def convert_to_bracket_notation_rec(self):
		if (len(self.children) > 0):
			s = "(" + self.symbol + " "

			for c in self.children:
				s += c.convert_to_bracket_notation_rec()

			s += ")"
		else:
			s = self.symbol

		return s


	def convert_to_normal_tree(self):
		self.convert_to_normal_tree_rec(self)

	## debinarize the tree based on the @-indicators
	## marking binarization nodes in the training-set
	def convert_to_normal_tree_rec(self, node):
		if (node.symbol[-1] == '@'):
			temp = node

			## for all children of an '@'-labeled node, set
			## the parent of every child to node.parent and
			## add that child to the children of node.parent
			for child in temp.children:
				child.parent = node.parent
				(node.parent).children = (node.parent).children + [child]

				self.convert_to_normal_tree_rec(child)

			## delete the link from node.parent to node and node itself
			del (node.parent).children[((node.parent).children).index(node)]
			del node
		else:
			for child in node.children:
				self.convert_to_normal_tree_rec(child)






## note: one-based, assumes a dummy element in non_term_symbols[0]
## the grammar *MUST* be in CNF (epsilon-free and all productions
## should have the form A --> BC or A --> a)
##
##   grammar_rules:    {"S": {("NP", "VP"): 15, ("PP", "S@"): 12}, ...}
##   non_term_symbols: ["S", "NP", "VP", ...]
##
def cyk_generate_grammar_productions(grammar_rules, non_term_symbols):
	## list of legal "A --> BC" rules
	L = []

	## nr. of non-terminal symbols
	N = len(non_term_symbols) - 1
	M = 0

	## upper bound is N + 1 because the
	## needed range is [1, N] (inclusive)
	for idx_sym_a in xrange(1, N + 1):
		for idx_sym_b in xrange(1, N + 1):
			for idx_sym_c in xrange(1, N + 1):
				sym_a  = non_term_symbols[idx_sym_a]
				sym_b  = non_term_symbols[idx_sym_b]
				sym_c  = non_term_symbols[idx_sym_c]
				sym_bc = (sym_b, sym_c)

				## check if rule SA --> SB SC is in the grammar
				## (independent of the grammar's representation)
				symbs_set_rhs = grammar_rules[sym_a]

				if (sym_bc in symbs_set_rhs):
					L.append((idx_sym_a, sym_a,  idx_sym_b, sym_b,  idx_sym_c, sym_c))

				M += 1

	print("[cyk_generate_grammar_productions] %d potential rules, %d actual rules" % (M, len(L)))
	return L

## implements the CYK and PCYK parsing algorithms
##
## note that the *order* of the non-terminal symbols is actually
## completely irrelevant to the parser logic; only the index of
## the start-symbol passed to it determines what kind of context-
## free (sub)structure the parser will (try to) recognize within
## the sentence
##
##   grammar_rules:      {"S": {("NP", "VP"): 15, ("PP", "S@"): 12}, ...}
##   sentence_list:      ["the", "green", "brown", "fox", ...]
##   start_symb_indices: [non_term_symbols.index("S"), ...]
##   non_term_symbols:   ["S", "NP", "VP", ...]
##   non_term_prods:     output of cyk_generate_grammar_productions(...)
##   prod_probs:         {"S": {("NP", "VP"): 0.123}, ...}
def cyk_parse_sentence(grammar_rules, sentence_list, start_symb_indices, non_term_symbols, non_term_prods, prod_probs):
	num_words     = (len(sentence_list) - 1)
	num_non_terms = (len(non_term_symbols) - 1)
	probabilistic = (len(prod_probs) > 0)
	parse_trees   = []

	print("[cyk_parse_sentence] num_words: %d, sentence_list: \"%s\"" % (num_words, sentence_list))

	T = None
	H = None

	## initialize the 3D tables
	if (probabilistic):
		T = [[[0.0   for k in xrange(num_non_terms + 1)] for j in xrange(num_words + 1)] for i in xrange(num_words + 1)]
		H = [[[None  for k in xrange(num_non_terms + 1)] for j in xrange(num_words + 1)] for i in xrange(num_words + 1)]
	else:
		T = [[[False for k in xrange(num_non_terms + 1)] for j in xrange(num_words + 1)] for i in xrange(num_words + 1)]
		H = [[[None  for k in xrange(num_non_terms + 1)] for j in xrange(num_words + 1)] for i in xrange(num_words + 1)]

	## base case
	##
	## upper bounds are N + 1 because the
	## needed ranges are [1, N] (inclusive)
	for i in xrange(1, num_words + 1):
		have_rule = False

		for idx_sym_a in xrange(1, num_non_terms + 1):
			symb_lhs  = non_term_symbols[idx_sym_a]
			symb_rhs  = (sentence_list[i], "")
			symbs_set = grammar_rules[symb_lhs]

			## ACHTUNG: conversion to one-element tuple
			## symb_rhs = sentence_list[i],

			if (symb_rhs in symbs_set):
				## print("\t[base] \"%s --> %s\" in grammar_rules" % (symb_lhs, symb_rhs))
				have_rule = True

				if (probabilistic):
					T[i][1][idx_sym_a] = prod_probs[symb_lhs][symb_rhs]
				else:
					T[i][1][idx_sym_a] = True

		if (not have_rule):
			print("\t[base] no rule for terminal symbol \"%s\", cannot parse sentence" % (sentence_list[i]))
			del T
			del H
			return parse_trees

	## recursive case
	for j in xrange(2, num_words + 1):
		for i in xrange(1, num_words - j + 2):
			for k in xrange(1, j):
				## print("\t[rec] substring w[i=%d] to w[i+j=%d] = %s" % (i, i + j, sentence_list[i: i + j]))

				for symbs_abc in non_term_prods:
					idx_sym_a = symbs_abc[0]; sym_a = symbs_abc[1]
					idx_sym_b = symbs_abc[2]; sym_b = symbs_abc[3]
					idx_sym_c = symbs_abc[4]; sym_c = symbs_abc[5]

					if (probabilistic):
						sym_bc = (sym_b, sym_c)
						rprob  = (T[i][k][idx_sym_b] * T[i + k][j - k][idx_sym_c]) * prod_probs[sym_a][sym_bc]

						if (rprob > T[i][j][idx_sym_a]):
							T[i][j][idx_sym_a] = rprob
							H[i][j][idx_sym_a] = (k,  idx_sym_b, sym_b,  idx_sym_c, sym_c)
					else:
						if (T[i][k][idx_sym_b]  and  T[i + k][j - k][idx_sym_c]):
							T[i][j][idx_sym_a] = True
							H[i][j][idx_sym_a] = (k,  idx_sym_b, sym_b,  idx_sym_c, sym_c)

							## print('\t\ti=%d j=%d   (A --> BC) = (%s --> %s)' % (i, j, sym_a, H[i][j][idx_sym_a]))

	for start_symb_idx in start_symb_indices:
		if ((T[1][num_words][start_symb_idx] == True) or (T[1][num_words][start_symb_idx] > 0.0)):
			parse_tree = cyk_parse_tree(None, non_term_symbols[start_symb_idx])

			cyk_extract_parse_tree_rec(T, H, sentence_list, parse_tree,  1, num_words, start_symb_idx)

			if (probabilistic):
				## parse_tree.logprob = cyk_parse_tree_logprob(parse_tree, prod_probs)
				## save a tree-traversal, the T-table already stores this value
				parse_tree.logprob = log(T[1][num_words][start_symb_idx])

			parse_trees.append(parse_tree)
			print("\tsentence produced a derivation for start-symbol %s" % (non_term_symbols[start_symb_idx]))

	del T
	del H
	return parse_trees

def cyk_extract_parse_tree_rec(T, H, s,  p,  i, j, m):
	if (H[i][j][m] != None):
		## LHS is needed for the non-probabilistic case
		assert(T[i][j][m] == True or T[i][j][m] > 0.0)

		span = H[i][j][m]
		k    = span[0]

		idx_sym_b = span[1]; sym_b = span[2]
		idx_sym_c = span[3]; sym_c = span[4]

		cl = cyk_parse_tree(p, sym_b); p.children.append(cl)
		cr = cyk_parse_tree(p, sym_c); p.children.append(cr)

		## cl spans from i     to i + k; length k
		## cr spans from i + k to j;     length j - k
		cyk_extract_parse_tree_rec(T, H, s,  cl,  i,     k,     idx_sym_b)
		cyk_extract_parse_tree_rec(T, H, s,  cr,  i + k, j - k, idx_sym_c)
	else:
		c = cyk_parse_tree(p, s[i])
		p.children.append(c)

def cyk_parse_tree_logprob(tree_node, prod_probs):
	if (len(tree_node.children) == 0):
		## if we have no children, we must be a terminal
		## node and do not contribute to the probability
		## (only *rules* make them, ie. parent ==> child
		## node relations)
		return 0.0

	rule_lhs = tree_node.symbol
	rule_rhs = [""] * len(tree_node.children)

	for i in xrange(len(tree_node.children)):
		rule_rhs[i] = (tree_node.children[i]).symbol

	rule_rhs = tuple(rule_rhs)
	tree_prb = log(prod_probs[rule_lhs][rule_rhs])

	for c in tree_node.children:
		tree_prb += cyk_parse_tree_logprob(c, prod_probs)

	return tree_prb

