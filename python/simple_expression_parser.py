import sys
import re

## if a sign is present then either sequence of digits
## before period or sequence after period must also be
## present, so the possibilities for constants are
##
##   (sign? number+ period? number*)   "5",  ".5",  "5.",  "5.5"
##   (sign? number* period? number+)  "+5", "+.5", "+5.", "+5.5"
##
## however, applying regexes for this creates ambiguity
##   1) if we scan for constants first,  "3+5" tokenizes as    [3,+5] instead of  [3,+,5]
##   2) if we scan for operators first, "+5*6" tokenizes as [+,5,*,6] instead of [+5,*,6]
##
## the proper solution would be to use lookahead, but we
## can sidestep the issue by not allowing signed numbers
## and substituting bin-ops by mon-ops where appropriate
## during tokenization (unary operators act like signs)
##
REGEX_TOKEN_BLANK = '[\s]*'
REGEX_TOKEN_S_NUM =                \
	'([+-]?[0-9]+\.?[0-9]*)' + '|' \
	'([+-]?[0-9]*\.?[0-9]+)'
REGEX_TOKEN_U_NUM =           \
	'([0-9]+\.?[0-9]*)' + '|' \
	'([0-9]*\.?[0-9]+)'
REGEX_TOKEN_MONOP = '([\+\-\~])'
REGEX_TOKEN_BINOP = '([\+\-\/\&\|\^\*\@])'
REGEX_TOKEN_L_PAR = '\('
REGEX_TOKEN_R_PAR = '\)'

TOKEN_REGEX = REGEX_TOKEN_MONOP + "|" + REGEX_TOKEN_BINOP + "|" + REGEX_TOKEN_L_PAR + "|" + REGEX_TOKEN_R_PAR + "|" + REGEX_TOKEN_U_NUM


TOKEN_EXPR      =  0 ## note: this is a "virtual" token
TOKEN_CONST     =  1
TOKEN_MONOP     =  2
TOKEN_BINOP     =  3
TOKEN_L_PAR     =  4
TOKEN_R_PAR     =  5

TOKEN_OPER_POS  =  6 ## unary
TOKEN_OPER_NEG  =  7 ## unary
TOKEN_OPER_NOT  =  8 ## unary, bitwise
TOKEN_OPER_ADD  =  9 ## binary
TOKEN_OPER_SUB  = 10 ## binary
TOKEN_OPER_MUL  = 11 ## binary
TOKEN_OPER_DIV  = 12 ## binary
TOKEN_OPER_POW  = 13 ## binary
TOKEN_OPER_AND  = 14 ## binary, bitwise
TOKEN_OPER_OR   = 15 ## binary, bitwise
TOKEN_OPER_XOR  = 16 ## binary, bitwise

TOKEN_OPER_SUBSTS = {
	TOKEN_OPER_ADD: TOKEN_OPER_POS,
	TOKEN_OPER_SUB: TOKEN_OPER_NEG,
}

## lower precedence value means operator binds stronger
OPER_PRECEDENCE_TABLE = {
	TOKEN_OPER_POS: 3,
	TOKEN_OPER_NEG: 3,
	TOKEN_OPER_NOT: 3,
	TOKEN_OPER_ADD: 4,
	TOKEN_OPER_SUB: 4,
	TOKEN_OPER_MUL: 2,
	TOKEN_OPER_DIV: 2,
	TOKEN_OPER_POW: 1,
	TOKEN_OPER_AND: 5,
	TOKEN_OPER_OR : 5,
	TOKEN_OPER_XOR: 5,
}

TEST_EXPRESSIONS = [
	"3*5",
	"(3*5)",
	"3+(4*5)",
	"(4*5)+3",
	"(4*5)+3-6",
	"((4*5)+3-6)",
	"(((4*5)+3-6))",
	"(3+(4*5))",
	"-----------1",
	"5*-----------1",
	"-----------1*5",
	"++++++++++++++-5",
	"-+++++++(3*8)+++++++-5",
	"-(-(-(-(-(-(-(-(-1))))))))*5",
	"((((((((((((8*3))))))))))))-1",
]

EXPRESSION_GRAMMAR = {
	## note: left-recursive
	TOKEN_EXPR: [
		[TOKEN_L_PAR, TOKEN_EXPR, TOKEN_R_PAR],
		[TOKEN_EXPR, TOKEN_BINOP, TOKEN_EXPR],
		[TOKEN_MONOP, TOKEN_EXPR],
		[TOKEN_CONST],
	],

	## terminals (these do not actually need to
	## be present when using a bottom-up parser)
	TOKEN_MONOP: [[]],
	TOKEN_BINOP: [[]],
	TOKEN_CONST: [[]],
}



def dummy_print(s): pass
def debug_print(s): print(s)

class ast_token:
	def __init__(self, t, v, i):
		self.set_type(t)
		self.set_value(v)
		self.set_index(i)

	def __repr__(self):
		return ("(t=%s, v=%s, i=%d)" % (self.type, self.value, self.index))

	def set_type(self, t): self.type = t
	def set_value(self, v): self.value = v
	def set_index(self, i): self.index = i
	def get_type(self): return self.type
	def get_value(self): return self.value
	def get_index(self): return self.index

class ast_node:
	## assumes a grammar with at most three tokens per non-terminal RHS
	def __init__(self, num_children = 3):
		self.set_nodes([None] * num_children)
		self.set_token(None)

	def set_nodes(self, nodes): self.nodes = nodes
	def set_node(self, idx, n): self.nodes[idx] = n
	def get_node(self, idx): return self.nodes[idx]

	def set_token(self, t): self.token = t
	def get_token(self): return self.token

	## note: could use this to generate instructions too
	def evaluate(self):
		if ((self.token).get_type() == TOKEN_CONST):
			assert((self.token).get_value() != None)
			return ((self.token).get_value())

		elif ((self.token).get_type() == TOKEN_MONOP):
			assert(self.nodes[0] == None and self.nodes[2] == None)
			assert(self.nodes[1] != None)

			mhs = (self.nodes[1]).evaluate()

			if ((self.token).get_value() == TOKEN_OPER_POS): return (+mhs)
			if ((self.token).get_value() == TOKEN_OPER_NEG): return (-mhs)
			if ((self.token).get_value() == TOKEN_OPER_NOT): return (~mhs)

			assert(False)

		elif ((self.token).get_type() == TOKEN_BINOP):
			assert(self.nodes[0] != None and self.nodes[2] != None)

			lhs = (self.nodes[0]).evaluate()
			rhs = (self.nodes[2]).evaluate()

			if ((self.token).get_value() == TOKEN_OPER_ADD): return (lhs  + rhs)
			if ((self.token).get_value() == TOKEN_OPER_SUB): return (lhs  - rhs)
			if ((self.token).get_value() == TOKEN_OPER_MUL): return (lhs  * rhs)
			if ((self.token).get_value() == TOKEN_OPER_DIV): return (lhs  / rhs)
			if ((self.token).get_value() == TOKEN_OPER_POW): return (lhs ** rhs)
			if ((self.token).get_value() == TOKEN_OPER_AND): return (int(lhs) & int(rhs))
			if ((self.token).get_value() == TOKEN_OPER_OR ): return (int(lhs) | int(rhs))
			if ((self.token).get_value() == TOKEN_OPER_XOR): return (int(lhs) ^ int(rhs))

			assert(False)

		return 0.0



def test_regexes(print_func):
	print_func("[regex_test][1]")

	s =   "+5"; m = re.match(REGEX_TOKEN_S_NUM, s); assert(m != None); print_func("\t%s, %s, %s" % (m.span(), m.group(), m.string))
	s =    "5"; m = re.match(REGEX_TOKEN_S_NUM, s); assert(m != None); print_func("\t%s, %s, %s" % (m.span(), m.group(), m.string))
	s =  "5." ; m = re.match(REGEX_TOKEN_S_NUM, s); assert(m != None); print_func("\t%s, %s, %s" % (m.span(), m.group(), m.string))
	s =  "5.5"; m = re.match(REGEX_TOKEN_S_NUM, s); assert(m != None); print_func("\t%s, %s, %s" % (m.span(), m.group(), m.string))

	print_func("[regex_test][2]")

	s =  "+.5"; m = re.match(REGEX_TOKEN_S_NUM, s); assert(m != None); print_func("\t%s, %s, %s" % (m.span(), m.group(), m.string))
	s =  "-.5"; m = re.match(REGEX_TOKEN_S_NUM, s); assert(m != None); print_func("\t%s, %s, %s" % (m.span(), m.group(), m.string))
	s = "-0.5"; m = re.match(REGEX_TOKEN_S_NUM, s); assert(m != None); print_func("\t%s, %s, %s" % (m.span(), m.group(), m.string))
	s = "+0.5"; m = re.match(REGEX_TOKEN_S_NUM, s); assert(m != None); print_func("\t%s, %s, %s" % (m.span(), m.group(), m.string))

	print_func("[regex_test][3]")

	s =  "+0."; m = re.match(REGEX_TOKEN_S_NUM, s); assert(m != None); print_func("\t%s, %s, %s" % (m.span(), m.group(), m.string))
	s =  "-0."; m = re.match(REGEX_TOKEN_S_NUM, s); assert(m != None); print_func("\t%s, %s, %s" % (m.span(), m.group(), m.string))
	s =   "+0"; m = re.match(REGEX_TOKEN_S_NUM, s); assert(m != None); print_func("\t%s, %s, %s" % (m.span(), m.group(), m.string))
	s =   "-0"; m = re.match(REGEX_TOKEN_S_NUM, s); assert(m != None); print_func("\t%s, %s, %s" % (m.span(), m.group(), m.string))

	print_func("[regex_test][4]")

	s =    "++5"; m = re.match(REGEX_TOKEN_S_NUM, s); assert(m == None)
	s =    "--5"; m = re.match(REGEX_TOKEN_S_NUM, s); assert(m == None)
	s =    "+-5"; m = re.match(REGEX_TOKEN_S_NUM, s); assert(m == None)
	s =    "-+5"; m = re.match(REGEX_TOKEN_S_NUM, s); assert(m == None)
	s =    "5++"; m = re.match(REGEX_TOKEN_S_NUM, s); assert(m != None); print_func("\t%s, %s, %s" % (m.span(), m.group(), m.string))
	s =   ".5++"; m = re.match(REGEX_TOKEN_S_NUM, s); assert(m != None); print_func("\t%s, %s, %s" % (m.span(), m.group(), m.string))
	s =   "5.++"; m = re.match(REGEX_TOKEN_S_NUM, s); assert(m != None); print_func("\t%s, %s, %s" % (m.span(), m.group(), m.string))
	s =  "5.0++"; m = re.match(REGEX_TOKEN_S_NUM, s); assert(m != None); print_func("\t%s, %s, %s" % (m.span(), m.group(), m.string))

	print_func("[regex_test][5]")

	s =  "+"; m = re.match(REGEX_TOKEN_BINOP, s); assert(m != None); print_func("\t%s, %s, %s" % (m.span(), m.group(), m.string))
	s =  "-"; m = re.match(REGEX_TOKEN_BINOP, s); assert(m != None); print_func("\t%s, %s, %s" % (m.span(), m.group(), m.string))
	s =  "*"; m = re.match(REGEX_TOKEN_BINOP, s); assert(m != None); print_func("\t%s, %s, %s" % (m.span(), m.group(), m.string))
	s =  "/"; m = re.match(REGEX_TOKEN_BINOP, s); assert(m != None); print_func("\t%s, %s, %s" % (m.span(), m.group(), m.string))
	s = "\\"; m = re.match(REGEX_TOKEN_BINOP, s); assert(m == None)
	s =  "&"; m = re.match(REGEX_TOKEN_BINOP, s); assert(m != None); print_func("\t%s, %s, %s" % (m.span(), m.group(), m.string))
	s = "&&"; m = re.match(REGEX_TOKEN_BINOP, s); assert(m != None); print_func("\t%s, %s, %s" % (m.span(), m.group(), m.string))
	s =  "|"; m = re.match(REGEX_TOKEN_BINOP, s); assert(m != None); print_func("\t%s, %s, %s" % (m.span(), m.group(), m.string))
	s = "||"; m = re.match(REGEX_TOKEN_BINOP, s); assert(m != None); print_func("\t%s, %s, %s" % (m.span(), m.group(), m.string))
	s =  "^"; m = re.match(REGEX_TOKEN_BINOP, s); assert(m != None); print_func("\t%s, %s, %s" % (m.span(), m.group(), m.string))
	s =  "a"; m = re.match(REGEX_TOKEN_BINOP, s); assert(m == None)


def scan_expr(expr_string, print_func):
	index = 0
	regex = re.compile(TOKEN_REGEX)
	tokens = []

	def get_ast_token(token_string, token_index):
		## note: parentheses tokens have no actual "value"
		if (token_string == '('): return (ast_token(TOKEN_L_PAR, None, token_index))
		if (token_string == ')'): return (ast_token(TOKEN_R_PAR, None, token_index))

		if (token_string == '~'): return (ast_token(TOKEN_MONOP, TOKEN_OPER_NOT, token_index))

		if (token_string == '+'): return (ast_token(TOKEN_BINOP, TOKEN_OPER_ADD, token_index))
		if (token_string == '-'): return (ast_token(TOKEN_BINOP, TOKEN_OPER_SUB, token_index))
		if (token_string == '*'): return (ast_token(TOKEN_BINOP, TOKEN_OPER_MUL, token_index))
		if (token_string == '/'): return (ast_token(TOKEN_BINOP, TOKEN_OPER_DIV, token_index))
		if (token_string == '@'): return (ast_token(TOKEN_BINOP, TOKEN_OPER_POW, token_index))
		if (token_string == '&'): return (ast_token(TOKEN_BINOP, TOKEN_OPER_AND, token_index))
		if (token_string == '|'): return (ast_token(TOKEN_BINOP, TOKEN_OPER_OR , token_index))
		if (token_string == '^'): return (ast_token(TOKEN_BINOP, TOKEN_OPER_XOR, token_index))

		## assume token represents a number
		return (ast_token(TOKEN_CONST, float(token_string), token_index))

	def parenthesize_tokens(tokens):
		num_levels = max(OPER_PRECEDENCE_TABLE.values()) + 1

		## add parentheses to encode operator precedence (Fortran-style)
		##
		## opening parentheses (NOTE: repair indices if we ever use them)
		par_tokens = [ast_token(TOKEN_L_PAR, None, -1) for i in xrange(num_levels)]

		for i in xrange(len(tokens)):
			token = tokens[i]

			if (token.get_type() == TOKEN_CONST):
				par_tokens += [token]
				continue

			elif (token.get_type() == TOKEN_L_PAR):
				par_tokens += [ast_token(TOKEN_L_PAR, None, -1) for i in xrange(num_levels)]
			elif (token.get_type() == TOKEN_R_PAR):
				par_tokens += [ast_token(TOKEN_R_PAR, None, -1) for i in xrange(num_levels)]

			## monops are a special case that can not be handled by this
			## scheme; they will *always* be considered by the parser to
			## have maximal precedence (no matter their table precedence
			## level) unless manual ()'s are added
			## e.g. -5^2 is parsed as (-5)^2=25 instead of -(5^2)=-25
			elif (token.get_type() == TOKEN_MONOP):
				par_tokens += [token]

			elif (token.get_type() == TOKEN_BINOP):
				prec_level = OPER_PRECEDENCE_TABLE[token.get_value()]

				par_tokens += [ast_token(TOKEN_R_PAR, None, -1) for i in xrange(prec_level)]
				par_tokens += [token]
				par_tokens += [ast_token(TOKEN_L_PAR, None, -1) for i in xrange(prec_level)]

		## closing parentheses
		par_tokens += [ast_token(TOKEN_R_PAR, None, -1) for i in xrange(num_levels)]
		return par_tokens

	def subst_token(token, substs, index, print_func):
		v = token.get_value()

		if (v in substs):
			w = substs[v]

			token.set_type(TOKEN_MONOP)
			token.set_value(w)

			print_func("[subst_token] index=%d binop(%d)-->monop(%d)" % (index, v, w))
			return True

		return False

	def substitute_tokens(tokens, print_func):
		if (len(tokens) < 2):
			return tokens

		## look for any binops that should probably be
		## monops (based on surrounding local context)
		prev_token = None
		next_token = None

		for index in xrange(0, len(tokens) - 1):
			curr_token = tokens[index    ]
			next_token = tokens[index + 1]

			if (curr_token.get_type() != TOKEN_BINOP):
				prev_token = curr_token
				continue

			if (prev_token == None):
				## binary operator as first token, substitute
				subst_token(curr_token, TOKEN_OPER_SUBSTS, index, print_func)
			else:
				## binary operator as i-th token, check context
				##
				## if context is "CONST [BINOP] ..." or "R_PAR [BINOP] ...", do nothing
				if (prev_token.get_type() == TOKEN_CONST or prev_token.get_type() == TOKEN_R_PAR):
					prev_token = curr_token
					continue

				## if context is "L_PAR [BINOP] ..." or "MONOP [BINOP] ...", substitute
				if (prev_token.get_type() == TOKEN_L_PAR or prev_token.get_type() == TOKEN_MONOP):
					subst_token(curr_token, TOKEN_OPER_SUBSTS, index, print_func)

				## if context is "[BINOP] CONST ..." or "[BINOP] BINOP ...", substitute
				## RHS is permitted because we do not have {pre,post}{in,de}crement ops
				if (next_token.get_type() == TOKEN_CONST or next_token.get_type() == TOKEN_BINOP):
					subst_token(curr_token, TOKEN_OPER_SUBSTS, index, print_func)

			prev_token = curr_token

		return tokens

	while (index < len(expr_string)):
		match = regex.match(expr_string[index: ])

		if (match != None):
			span = match.span()
			token = match.group()

			print_func("[scan_expr] index=%d span=%s token=\"%s\"" % (index, span, token))

			index += (span[1] - span[0])
			tokens += [get_ast_token(token, index)]
		else:
			index += 1

	tokens = substitute_tokens(tokens, print_func)
	tokens = parenthesize_tokens(tokens)

	print_func("[scan_expr][\"%s\"]" % expr_string)
	for token in tokens:
		print_func("\t%s" % token)

	return tokens



def print_ast(tree_node, depth, print_func):
	if (tree_node == None):
		return

	if (tree_node.get_node(1) != None):
		## non-binary AST if this node has a middle-child
		print_ast(tree_node.get_node(2), depth + 1, print_func)
		print_ast(tree_node.get_node(1), depth + 1, print_func)
		print_ast(tree_node.get_node(0), depth + 1, print_func)
	else:
		print_ast(tree_node.get_node(2), depth + 1, print_func)
		print_func("%s%s" % ('\t' * depth, tree_node.get_token()))
		print_ast(tree_node.get_node(0), depth + 1, print_func)


def construct_ast(span_table, tree_node, scanned_tokens, span_min_idx, span_max_idx, depth):
	assert(span_min_idx <= span_max_idx)

	span = span_table[span_min_idx][span_max_idx]

	## check if (min_idx, max_idx) covers a subspan
	if (span == None):
		return

	span_lhs = span[0]
	span_rhs = span[1]
	rule_rhs = span[2]

	if (span_lhs == span_rhs):
		## unary rule (pre-terminal)
		const_token_indx = span_lhs[0]
		const_token_inst = scanned_tokens[const_token_indx]

		assert(len(rule_rhs) == 1)
		assert(tree_node.get_node(0) == None)
		assert(tree_node.get_node(2) == None)
		## leafs must have concrete token-types
		assert(rule_rhs[0] != TOKEN_EXPR)
		assert(rule_rhs[0] == TOKEN_CONST)
		assert(rule_rhs[0] == const_token_inst.get_type())
		assert(tree_node.get_token() == None)

		tree_node.set_token(ast_token(const_token_inst.get_type(), const_token_inst.get_value(), const_token_inst.get_index()))
	else:
		## n-ary rule (non-terminal); note that these
		## assertions are entirely grammar-dependent
		assert(len(rule_rhs) == 2 or len(rule_rhs) == 3)
		assert(rule_rhs[1] != TOKEN_CONST)

		if (len(rule_rhs) == 2):
			## e.g. EXPR --> [MONOP, EXPR]
			monop_token_indx = span_lhs[0]
			monop_token_type = rule_rhs[0]
			monop_token_inst = scanned_tokens[monop_token_indx]

			assert(rule_rhs[0] == TOKEN_MONOP)
			assert(monop_token_type == monop_token_inst.get_type())

			m_child_node = ast_node()
			construct_ast(span_table, m_child_node, scanned_tokens, span_rhs[0], span_rhs[1], depth + 1)

			tree_node.set_token(ast_token(monop_token_inst.get_type(), monop_token_inst.get_value(), monop_token_inst.get_index()))
			tree_node.set_node(1, m_child_node)

		if (len(rule_rhs) == 3):
			## e.g. EXPR --> [EXPR, BINOP, EXPR]
			lhs_terminal = (span_lhs[0] == span_lhs[1])
			rhs_terminal = (span_rhs[0] == span_rhs[1])
			recurse_span = (span_rhs[0] >  span_lhs[1])
			recurse_expr = (rule_rhs[1] == TOKEN_EXPR)

			if ((lhs_terminal and rhs_terminal) and (recurse_span and recurse_expr)):
				## this can only be reached for (L_PAR,EXPR,R_PAR) RHS rules
				##
				## "drop down" through the middle span element, peeling off
				## the parentheses since they are not useful tokens to keep
				## around in the AST
				##
				## NOTE: like the asserts above, this step is grammar-dependent
				construct_ast(span_table, tree_node, scanned_tokens, span_lhs[0] + 1, span_rhs[0] - 1, depth + 1)
			else:
				## this should only be reached for (EXPR,BINOP,EXPR) RHS rules
				assert(rule_rhs[1] == TOKEN_BINOP)
				assert(span_lhs[1] + 1 == span_rhs[0] - 1)

				binop_token_indx = span_lhs[1] + 1
				binop_token_type = rule_rhs[1]
				binop_token_inst = scanned_tokens[binop_token_indx]

				## tokens are filled in by next recursive level
				l_child_node = ast_node()
				r_child_node = ast_node()

				construct_ast(span_table, l_child_node, scanned_tokens, span_lhs[0], span_lhs[1], depth + 1)
				construct_ast(span_table, r_child_node, scanned_tokens, span_rhs[0], span_rhs[1], depth + 1)

				assert(scanned_tokens[span_lhs[1] + 1] == scanned_tokens[span_rhs[0] - 1])
				assert(binop_token_type == binop_token_inst.get_type())

				tree_node.set_token(ast_token(binop_token_inst.get_type(), binop_token_inst.get_value(), binop_token_inst.get_index()))
				tree_node.set_node(0, l_child_node)
				tree_node.set_node(2, r_child_node)


def parse_expr(scanned_tokens, grammar_rules, print_func):
	expr_stack = []
	indx_stack = []
	span_table = [ [None for j in xrange(len(scanned_tokens))] for i in xrange(len(scanned_tokens)) ]
	parse_tree = ast_node()

	## simple shift-reduce (stack-based, bottom-up) parser
	for token_idx in xrange(len(scanned_tokens)):
		token = scanned_tokens[token_idx]

		expr_stack = [token.get_type()] + expr_stack
		indx_stack = [(token_idx, token_idx)] + indx_stack

		min_stack_idx = 0
		max_stack_idx = len(expr_stack)

		while (min_stack_idx < max_stack_idx):
			for rule_lhs in grammar_rules:
				## will be empty for terminals
				rule_rhs_list = grammar_rules[rule_lhs]

				for rule_rhs in rule_rhs_list:
					sub_expr = expr_stack[ 0: min_stack_idx + 1 ]
					sub_expr.reverse()

					if (rule_rhs != sub_expr):
						continue

					## reduce sub-expression part of stack (matching rhs) to lhs
					expr_stack = expr_stack[ min_stack_idx + 1: ]
					expr_stack = [rule_lhs] + expr_stack

					## note: the lhs-interval is [(a,b), (c,d)] so we want a and d
					rule_min_idx = indx_stack[min_stack_idx][0]
					rule_max_idx = indx_stack[            0][1]

					## remember the sub-expression extents and corresponding tokens
					span_table[rule_min_idx][rule_max_idx] = (indx_stack[min_stack_idx], indx_stack[0], rule_rhs)

					indx_stack = indx_stack[ min_stack_idx + 1: ]
					indx_stack = [(rule_min_idx, rule_max_idx)] + indx_stack

					## reset indices after reducing
					min_stack_idx = 0
					max_stack_idx = len(expr_stack)

			min_stack_idx += 1

	## parse is succesful if final stack contains only TOKEN_EXPR
	if (len(expr_stack) == 1 and expr_stack[0] == TOKEN_EXPR):
		print_func("[parse_expr]")
		construct_ast(span_table, parse_tree, scanned_tokens, 0, len(scanned_tokens) - 1, 0)
		print_ast(parse_tree, 1, print_func)
		return parse_tree

	del parse_tree
	return None


def emit_instrs(tree_node, print_func):
	if (tree_node == None):
		return []

	## generate "bytecode" for some trivial VM
	instrs = emit_instrs_rec(tree_node, 0)

	print_func("[emit_instrs]")
	for instr in instrs:
		print_func("\t%s" % instr)

	return instrs

def emit_instrs_rec(tree_node, depth):
	assert(tree_node != None)

	instrs = []
	token = tree_node.get_token()

	if (tree_node.get_node(0) == None and tree_node.get_node(2) == None):
		assert(token.get_type() == TOKEN_CONST or token.get_type() == TOKEN_MONOP)

		## if left and right child are nil, we are a
		## constant leaf or a unary node (depends on
		## middle child)
		if (tree_node.get_node(1) == None):
			instrs += ["INSTR_PUSH_CONST %s" % (token.get_value())]
		else:
			## node represents a unary operator
			instrs += emit_instrs_rec(tree_node.get_node(1), depth + 1)
			instrs += ["INSTR_EXEC_MONOP %s" % (token.get_value())]
	else:
		## otherwise we are a binary node
		assert(token.get_type() == TOKEN_BINOP)
		assert(tree_node.get_node(0) != None and tree_node.get_node(2) != None)

		instrs += emit_instrs_rec(tree_node.get_node(0), depth + 1)
		instrs += emit_instrs_rec(tree_node.get_node(2), depth + 1)
		instrs += ["INSTR_EXEC_BINOP %s" % (token.get_value())]

	return instrs



def read_parse_expr(expr_string, print_func):
	expr_tokens = scan_expr(expr_string, print_func)
	expr_astree = parse_expr(expr_tokens, EXPRESSION_GRAMMAR, print_func)
	expr_instrs = emit_instrs(expr_astree, print_func)
	return (expr_astree, expr_instrs)

def repl():
	while (True):
		expr_string = raw_input("<< ")

		if (len(expr_string) == 0):
			continue
		if (expr_string[0] == 'q'):
			break

		parsed_expr = read_parse_expr(expr_string, dummy_print)
		expr_astree = parsed_expr[0]

		if (expr_astree != None):
			print(">> %f" % expr_astree.evaluate())
			continue

		print(">> invalid expression")

	return 0

def test(argv):
	try:
		expr_string = TEST_EXPRESSIONS[int(argv[1]) % len(TEST_EXPRESSIONS)]
	except ValueError:
		expr_string = argv[1]

	parsed_expr = read_parse_expr(expr_string, debug_print)
	expr_astree = parsed_expr[0]
	expr_instrs = parsed_expr[1]

	if (expr_astree != None):
		print("[test] expression value: %f (bytecode: %s)" % (expr_astree.evaluate(), expr_instrs))
	else:
		print("[test] invalid expression")

	return 0

def main(argc, argv):
	if (argc <= 1):
		test_regexes(debug_print)
		return 0

	## interactive mode
	if (argv[1] == "--i"):
		return (repl())

	return (test(argv))

sys.exit(main(len(sys.argv), sys.argv))

