<?php
function PolynomialLeastSquaresFit($degree, $xvals, $yvals) {
    $num_points = count($xvals);
	$poly_params = array();

	for ($row = 0; $row <= $degree; $row++) {
		// find the coefficients for the columns
		for ($col = 0; $col <= $degree; $col++) {
			// reset total
			$total = 0;

			// find SUM(x[i]^(row + col)) over all i
			for ($i = 0; $i < $num_points; $i++)
				$total += pow($xvals[$i], $row + $col);

			$coeffs[$row][$col] = $total;
		}

		// find SUM(y[i] * x[i]^n)
		$total = 0;
		for ($i = 0; $i < $num_points; $i++) {
			$total += ($yvals[$i] * pow($xvals[$i], $row));
		}

		$coeffs[$row][$degree + 1] = $total;
	}

	// now attempt Gauss-Jordan elimination
	$max_row = count($coeffs);
	$max_col = count($coeffs[0]);
    
	for ($row = 0; $row < $max_row; $row++) {
		$factor = $coeffs[$row][$row];

		if (abs($factor) < 0.001) {
			// make sure coeffs[row][row] != 0
			for ($i = $row + 1; $i < $max_row; $i++) {
				if (abs($coeffs[$i][$row] > 0.001)) {
					// switch rows <i> and <row>
					for ($j = 0; $j < $max_col; $j++ ) {
						$tmp = $coeffs[$row][$j];
						$coeffs[$row][$j] = $coeffs[$i][$j];
						$coeffs[$i][$j]   = $tmp;
					}

					$factor = $coeffs[$row][$row];
				}
			}

			if (abs($factor) < 0.001) {
				// no solution possible, return empty vector
				return $poly_params;
			}
		}

		// divide each entry in <row> by the coeffs[row][row] factor
		for ($i = 0; $i < $max_col; $i++) {
			$coeffs[$row][$i] /= $factor;
		}

		// subtract <row> from all others
		for ($i = 0; $i < $max_row; $i++) {
			if ($i != $row) {
				$factor = $coeffs[$i][$row];

				for ($j = 0; $j < $max_col; $j++) {
					$coeffs[$i][$j] -= ($factor * $coeffs[$row][$j]);
				}
			}
		}
	}

	// solution exists, so copy the last column of coefficients
	for ($row = 0; $row <= $degree; $row++) {
		$poly_params[$row] = $coeffs[$row][$degree + 1];
	}

	return $poly_params;
}

$xvals = array();
$yvals = array();

$xvals[ 0] = 3300; $yvals[ 0] =  0;
$xvals[ 1] = 3170; $yvals[ 1] = 10;
$xvals[ 2] = 3000; $yvals[ 2] = 20;
$xvals[ 3] = 2850; $yvals[ 3] = 30;
$xvals[ 4] = 2650; $yvals[ 4] = 40;
$xvals[ 5] = 2520; $yvals[ 5] = 50;
$xvals[ 6] = 2250; $yvals[ 6] = 60;
$xvals[ 7] = 2000; $yvals[ 7] = 72;
$xvals[ 8] = 1800; $yvals[ 8] = 70;
$xvals[ 9] = 1600; $yvals[ 9] = 66;
$xvals[10] = 1420; $yvals[10] = 62;
$xvals[11] = 1300; $yvals[11] = 70;
$xvals[12] = 1000; $yvals[12] = 84;
$xvals[13] =  900; $yvals[13] = 98;

// LSQ-fit an n-th order polynomial to the data
// test-data lies mostly on a line, so pick n=1
//
// note that the function parameters are actually
// returned in reverse order: p[0] = b, p[1] = a
//
$params = PolynomialLeastSquaresFit(1, $xvals, $yvals);

print_r($params);
?>

