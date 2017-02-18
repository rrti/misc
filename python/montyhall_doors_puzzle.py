import random

def simulate_games(num_games, num_doors):
	## TODO: generalize (host opens N-2 dummy doors then offers switch)
	assert(num_doors == 3)

	game_doors   = range(num_doors)
	player_doors = range(num_doors)
	switch_doors = True

	num_games_won  = 0
	num_games_lost = 0

	for i in xrange(num_games):
		player_door = random.choice(game_doors)
		prize_door = random.choice(game_doors)

		if (player_door == prize_door):
			game_doors.remove(prize_door)
		else:
			game_doors.remove(player_door)
			game_doors.remove(prize_door)

		host_door = random.choice(game_doors)

		if (switch_doors):
			## if player uses the switch-strategy, remove his
			## initial choice and the door(s if N > 3) already
			## opened by host, then pick the remaining
			##
			## if player initially picks prize door (33% prob.), switching always loses
			## if player initially picks dummy door (66% prob.), switching always wins!
			player_doors.remove(player_door)
			player_doors.remove(host_door)
			player_door = player_doors[0]
			player_doors = range(num_doors)

		if (player_door == prize_door):
			num_games_won += 1
		else:
			num_games_lost += 1

		game_doors = range(num_doors)


	winPerc = (float(num_games_won) / num_games) * 100
	lossPerc = (float(num_games_lost) / num_games) * 100

	print("[simulate] wins: %d (%.2f%%), losses: %d (%.2f%%)" % (num_games_won, winPerc, num_games_lost, lossPerc))

def analyze_probabilities(num_doors):
	assert(num_doors >= 3)

	if (False):
		## initial probability per door
		door_probs = [1.0 / num_doors] * num_doors

		for door_idx in xrange(num_doors - 1):
			## show that the last door to be opened (N-1) will
			## slowly accumulate all probability mass as every
			## non-prize door (except one) is visited
			print("[analyze] current_door=%d prob_per_door=%.3f" % (door_idx, door_probs[num_doors - 1]))

			## denominator represents the remaining number
			## of unopened "doors" behind which prize could
			## still be
			##
			## the probability for all remaining doors gets
			## updated each time a new one gets opened, whose
			## own probability is then set to 0
			num_nonvisited_doors = num_doors - (door_idx + 1)
			cur_door_probability = door_probs[door_idx] / num_nonvisited_doors

			door_probs[door_idx] = 0.0

			## redistribute probability mass from this door
			## over all remaining unopened doors uniformly
			for nxt_door_idx in xrange(door_idx + 1, num_doors):
				door_probs[nxt_door_idx] += cur_door_probability

		## door N-2 is the non-prize dummy and left unopened
		assert(door_probs[num_doors - 2] < 0.0000001)
		assert(door_probs[num_doors - 1] > 0.9999999)
	else:
		prob_per_door = (1.0 / num_doors)
		doors_visited = 0

		while ((doors_visited < num_doors - 1) and (prob_per_door < 1.0)):
			print("[analyze] doors_visited=%d prob_per_door=%.3f" % (doors_visited, prob_per_door))

			doors_visited += 1
			prob_per_door += (prob_per_door / (num_doors - doors_visited))

		assert(prob_per_door > 0.9999999)

simulate_games(10000, 3)
analyze_probabilities(3)

