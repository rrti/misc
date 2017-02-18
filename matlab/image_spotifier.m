function spotify_image(src_file, dst_file, diameter)
	%% load image and convert to grayscale
	src_img = rgb2gray(im2double(imread(src_file)));

	%% calculate the dimensions of the output image
	n_rows = round(size(src_img, 1) / diameter);
	n_cols = round(size(src_img, 2) / diameter);

	%% resize, normalize and invert the source image
	src_img = imresize(src_img, [n_rows n_cols], 'bicubic');
	src_img = (src_img - min(src_img(:))) / (max(src_img(:) - min(src_img(:))));
	src_img = 1 - src_img;

	%% create an empty destination image
	dst_img = zeros((n_rows + 1) * diameter, (n_cols + 1) * diameter);

	%% draw the dots
	for i = 1: n_cols
		for j = 1: n_rows
			x = i * diameter;
			y = j * diameter;

			%% radius of each dot is determined by darkness of original pixel
			radius_sq = (src_img(j, i) * diameter*diameter) / pi
			dst_img = add_image_spot(dst_img, x, y, sqrt(radius_sq));
		end
	end

	%% if output image is a .png, store alpha
	%% otherwise just store a grayscale image
	if (strcmpi(dst_file(end - 3: end), '.png'))
		imwrite(zeros(size(dst_img)), dst_file, 'Alpha', dst_img);
	else
		imwrite(1 - dst_img, dst_file);
	end
end

function dst_img = add_image_spot(dst_img, x, y, r)
	%% determine spot dimensions in pixels
	w = ceil(r) + (x - floor(x)) - 1;
	h = ceil(r) + (y - floor(y)) - 1;

	%% create a meshgrid
	[X, Y] = meshgrid(-h: h, -w: w);

	%% create the spot (half-sized to reduce aliasing)
	D = r - sqrt(X .^ 2 + Y .^ 2);
	C = min(max(D * 0.5, 0), 1);

	% calculate absolute position of spot in dst_img
	X = x + X;
	Y = y + Y;

	%% calculate pixel indices in dst_img
	idx = (X - 1) * size(I, 1) + Y;

	%% write ("blit") spot pixels into dst_img
	dst_img(idx) = max(dst_img(idx), C);
end

