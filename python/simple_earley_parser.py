import os
import sys

def print_parse_tree(parse_expr):
	## peel off the "(gamma ... )"
	parse_expr = parse_expr[6: -1]
	print_expr = ""
	num_parens = 0

	for idx in xrange(parse_expr.index('('), len(parse_expr)):
		if (parse_expr[idx] == '('):
			num_parens += 1

			print("%s" % print_expr)
			print("%s" % ("  " * num_parens)),

			print_expr = parse_expr[idx]
		else:
			if (parse_expr[idx] == ')'):
				assert(num_parens > 0)
				num_parens -= 1

			print_expr += parse_expr[idx]

	print("%s\n" % print_expr)



def earley_parse(grammar, sentence):
	num_bins = len(sentence) + 1

	## chart to keep track of partial sentence-parses
	## the entry at index i stores a list of all rules
	## possible at that point of the format
	##
	##   [lhs, [rhs], min_idx, max_idx, [parse-span]]
	##
	chart = [  [] for i in xrange(num_bins)  ]

	## add start-symbol (assumed to be 'S') to first bin
	##
	## the dot in [rhs] signifies how much of RHS has
	## been successfully matched to the input sentence
	##
	chart[0] = [  ["gamma", ['.', 'S'], 0, 0, []]  ]

	## for each rule in each bin, pass chart and rule to
	## predictor, scanner, and completer functions which
	## update the chart according to the rule
	##
	for bin_idx in xrange(num_bins):
		cur_rule_idx = 0
		max_rule_idx = len(chart[bin_idx])

		while (cur_rule_idx < max_rule_idx):
			## NOTE: chart[bin_idx] is grown in-place (except by the scanner)
			chart = earley_predict(grammar, chart, bin_idx, cur_rule_idx)
			chart = earley_lexscan(grammar, chart, bin_idx, cur_rule_idx, sentence)
			chart = earley_complete(chart, bin_idx, cur_rule_idx)

			cur_rule_idx += 1
			max_rule_idx = len(chart[bin_idx])

	## print all complete possible parses
	num_parses = 0

	for rule in chart[num_bins - 1]:
		if (rule[0] == "gamma"):
			num_parses += 1
			parse_expr = rule[4][0]

			print("[earley_parse][parse=%d] expr=\"%s\"" % (num_parses, parse_expr))
			print_parse_tree(parse_expr)

	if (num_parses == 0):
		print("[earley_parse] sentence \"%s\" is not in grammar" % sentence)
		return 1

	return 0

    

def add_chart_rule(rule, chart_bin):
	if (not (rule in chart_bin)):
		## filter duplicates
		chart_bin.append(rule)

	return chart_bin



## if there is a non-terminal to the right of the dot, add new rule
## for each possible expansion of that non-terminal to the current
## chart bin
##
def earley_predict(grammar, chart, bin_idx, rule_idx):
	bin_rule = chart[bin_idx][rule_idx]

	## if dot is already at end of RHS, there are no possible expansions
	if (bin_rule[1][-1] == '.'):
		return chart

	## extract RHS sub-list to the right of the dot
	cur_rhs = bin_rule[1][ bin_rule[1].index('.') + 1 ]   

	## find expansions in binary productions
	if (cur_rhs in grammar[0].keys()):
		for nxt_rhs in grammar[0][cur_rhs]:
			## add progress-marker dot to rule if necessary
			if (nxt_rhs[0] != '.'):
				nxt_rhs.insert(0, '.')

			chart_bin_rule = [cur_rhs, nxt_rhs, bin_idx, bin_idx, []]
			chart[bin_idx] = add_chart_rule(chart_bin_rule, chart[bin_idx])

	## find expansions in unary productions
	if (not (cur_rhs in grammar[1].keys())):
		return chart

	for nxt_rhs in grammar[1][cur_rhs]:
		chart_bin_rule = [cur_rhs, ['.', nxt_rhs], bin_idx, bin_idx, []]
		chart[bin_idx] = add_chart_rule(chart_bin_rule, chart[bin_idx])

	return chart


## if there is a POS tag to the right of the dot, check if that POS
## can generate the next word in the input sentence, and if so move
## dot forward and add the updated rule to the next chart bin
##
def earley_lexscan(grammar, chart, bin_idx, rule_idx, sentence):
	bin_rule = chart[bin_idx][rule_idx]
	trm_syms = grammar[2]

	## if dot is already at end of RHS, there are no possible expansions
	if (bin_rule[1][-1] == '.'):
		return chart

	## extract RHS sub-list to the right of the dot
	cur_rhs = bin_rule[1][ bin_rule[1].index('.') + 1 ]

	## find associated terminals: if next word in input sentence can be
	## produced by a RHS argument, then add that rule to the next chart
	## bin
	if (not (cur_rhs in trm_syms.keys())):
		return chart

	if (bin_idx >= len(sentence)):
		return chart
	if (not (sentence[bin_idx] in trm_syms[cur_rhs])):
		return chart

	## begin to form the partial parse-tree
	parse_span_exp = ['(' + cur_rhs + ' ' + sentence[bin_idx] +')']

	chart_bin_rule = [cur_rhs, [sentence[bin_idx], '.'], bin_idx, (bin_idx + 1), parse_span_exp]
	chart[bin_idx + 1] = add_chart_rule(chart_bin_rule, chart[bin_idx + 1])

	return chart


## if a rule has been completed, examine all previous rules in the
## chart bin where the rule was started and if one of these is still
## possible given the completion of the current rule, generate a new
## state with that rule in the current bin index
##
## note: this relies on the assumption of a CNF grammar!
##
def earley_complete(chart, bin_idx, rule_idx):
	bin_rule = chart[bin_idx][rule_idx]

	## only work on completed rules (dot at the end)
	if (bin_rule[1][-1] != '.'):
		return chart

	rule_lhs_sym = bin_rule[0]
	rule_min_idx = bin_rule[2]
	cur_span_exp = bin_rule[4][0]

	## iterate over all rules in chart bin corresponding
	## to the starting index of the completed rule symbol
	for prv_bin_rule in chart[rule_min_idx]:
		prv_rule_lhs = prv_bin_rule[0]
		prv_rule_rhs = prv_bin_rule[1]
		prv_dot_idx = prv_rule_rhs.index('.')

		## ignore if previous rule has already completed
		if (len(prv_rule_rhs) == (prv_dot_idx + 1)):
			continue

		## ignore if next argument of previous RHS is completed
		if (prv_rule_rhs[prv_dot_idx + 1] != rule_lhs_sym):
			continue

		if (len(prv_rule_rhs) == 2):
			## if rule is unary, add the ()
			nxt_span_exp = ['(' + prv_rule_lhs + ' ' + cur_span_exp + ')']
		elif (prv_dot_idx == 0):
			## first argument of a binary RHS has been completed, add (
			nxt_span_exp = ['(' + prv_rule_lhs + ' ' + cur_span_exp]
		else:
			## otherwise the second argument must have been completed, add )
			nxt_span_exp = [prv_bin_rule[4][0] + ' ' + cur_span_exp + ')']

		parsed_rhs  = prv_rule_rhs[0: prv_dot_idx]
		parsed_rhs += [rule_lhs_sym]
		parsed_rhs += ['.']

		nxt_rule_rhs = parsed_rhs + prv_rule_rhs[prv_rule_rhs.index(rule_lhs_sym) + 1: ]

		## create [LHS, [RHS with dot moved], old start index, current index, [new span]] entry
		chart_bin_rule = [prv_rule_lhs, nxt_rule_rhs, prv_bin_rule[2], bin_idx, nxt_span_exp]
		chart[bin_idx] = add_chart_rule(chart_bin_rule, chart[bin_idx])

	return chart




def default_grammar():
	g = []
	g += ["S -> NP VP\n"]
	g += ["S -> X1 VP\n"]
	g += ["X1 -> AUX NP\n"]
	g += ["S -> VP\n"]
	g += ["NP -> PRONOUN\n"]
	g += ["NP -> PROPERNOUN\n"]
	g += ["NP -> DET NOMINAL\n"]
	g += ["NOMINAL -> NOUN\n"]
	g += ["NOMINAL -> NOMINAL NOUN\n"]
	g += ["NOMINAL -> NOMINAL PP\n"]
	g += ["VP -> VERB\n"]
	g += ["VP -> VERB NP\n"]
	g += ["VP -> X2 PP\n"]
	g += ["X2 -> VERB NP\n"]
	g += ["VP -> VERB PP\n"]
	g += ["VP -> VP PP\n"]
	g += ["PP -> PREPOS NP\n"]
	g += ["DET -> that\n"]
	g += ["DET -> this\n"]
	g += ["DET -> a\n"]
	g += ["DET -> the\n"]
	g += ["NOUN -> book\n"]
	g += ["NOUN -> flight\n"]
	g += ["NOUN -> meal\n"]
	g += ["NOUN -> money\n"]
	g += ["PRONOUN -> i\n"]
	g += ["PRONOUN -> she\n"]
	g += ["PRONOUN -> he\n"]
	g += ["PRONOUN -> me\n"]
	g += ["PROPERNOUN -> houston\n"]
	g += ["PROPERNOUN -> nwa\n"]
	g += ["AUX -> does\n"]
	g += ["AUX -> do\n"]
	g += ["PREPOS -> from\n"]
	g += ["PREPOS -> to\n"]
	g += ["PREPOS -> on\n"]
	g += ["PREPOS -> near\n"]
	g += ["PREPOS -> through\n"]
	g += ["PREPOS -> for\n"]
	g += ["VERB -> book\n"]
	g += ["VERB -> include\n"]
	g += ["VERB -> prefer\n"]
	return g

def read_grammar(grammar_path):
	grammar_list = []

	if (not os.path.isfile(grammar_path)):
		grammar_list = default_grammar()
	else:
		grammar_file = open(grammar_path, 'r')
		grammar_list = grammar_file.readlines()
		grammar_file = f.close()

	bin_rules = {} ## rules with two-argument RHS (S --> NP VP, etc)
	una_rules = {} ## rules with one-argument RHS (unit productions)
	trm_symbs = {} ## terminal symbols

	## store each rule in corresponding dictionary
	## format is {lhs : [list of possible RHS's] }
	##
	for grammar_rule in grammar_list:
		## ignore comments
		if (grammar_rule[0] == '#'):
			continue

		rule = grammar_rule.split('->')
		rule_lhs = rule[0].strip(' ')
		rule_rhs = rule[1].strip(' ').strip('\n')

		if (len(rule_rhs.split(' ')) > 1):
			if (rule_lhs in bin_rules.keys()):
				bin_rules[rule_lhs] += [rule_rhs.split(' ')]
			else:
				bin_rules[rule_lhs] = [rule_rhs.split(' ')]

		elif (rule_rhs[0].isupper()):
			if (rule_lhs in una_rules.keys()):
				una_rules[rule_lhs] += [rule_rhs]
			else:
				una_rules[rule_lhs] = [rule_rhs]

		else:
			if (rule_lhs in trm_symbs.keys()):
				trm_symbs[rule_lhs] += [rule_rhs]
			else:
				trm_symbs[rule_lhs] = [rule_rhs]

	return [bin_rules, una_rules, trm_symbs]



def main(argc, argv):
	if (argc < 3):
		print("[main] usage: python %s <grammar.txt> <sentence>" % argv[0])
		return 1

	## find all possible parses of <sentence> given <grammar>
	## note: grammar must be specified in Chomsky Normal Form
	return (earley_parse(read_grammar(argv[1]), argv[2].split(' ')))

sys.exit(main(len(sys.argv), sys.argv))

