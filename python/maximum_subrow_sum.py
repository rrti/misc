def PrintTable(m):
	s = ""
	for i in range(len(m)):
		for j in range(len(m[0])):
			s += (str(m[i][j]) + '\t')
		s += '\n'
	print("%s" % s)

def MaxSubRowSum(s):
	n = len(s)
	m = [[0] * n for i in range(n)]
	p = 0

	for i in range(0, n):
		m[i][i] = s[i]

		for j in range(i + 1, n):
			m[i][j] = m[i][j - 1] + s[j]
			p = max(m[i][j], p)

	return (m, p)

S = [31, -41, 59, 26, -53, 58, 97, -23, 84, -100]
T = MaxSubRowSum(S)

PrintTable(T[0])
print("[MaxSubRowSum]%s=%d" % (S, T[1]))

