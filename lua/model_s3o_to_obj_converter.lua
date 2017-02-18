local src_model_filename = "objects3d/model.s3o"
local dst_model_filename = "objects3d/model.obj"

local raw_model_data = {
	bytes = nil,
	index = -1,
}

local UnpackU32 = VFS.UnpackU32
local UnpackS32 = VFS.UnpackS32
local UnpackF32 = VFS.UnpackF32

local strsub  = string.sub
local strfind = string.find
local strfmt  = string.format



local function read_uint32(data)
	local val = UnpackU32(data.bytes, data.index)
	data.index = data.index + 4
	return val
end

local function read_float32(data)
	local val = UnpackF32(data.bytes, data.index)
	data.index = data.index + 4
	return val
end


local function read_string(data, len)
	local val = strsub(data.bytes, data.index, len)
	data.index = data.index + len
	return val
end

local function read_sub_string(data, pos)
	local startIdx, endIdx, val = strfind(data.bytes, '([^%z]*)', pos)

	-- include the terminator
	data.index = data.index + (endIdx - startIdx + 2)

	return (tostring(val))
end




local function print_piece_tree(piece, indent)
	indent = indent or ''

	if (#piece.childs > 0) then
		print(strfmt('[%3i]  %s%s {', piece.id, indent, piece.name))
	else
		print(strfmt('[%3i]  %s%s', piece.id, indent, piece.name))
	end

	for _, child in ipairs(piece.childs) do
		print_piece_tree(child, indent .. '  ')
	end

	if (#piece.childs > 0) then
		print('.....  '..indent..'}')
	end
end

local function set_piece_bounds(piece)
	piece.mins = {}
	piece.maxs = {}

	for d = 1, 3 do
		local min =  math.huge
		local max = -math.huge

		for _, v in ipairs(piece.vertices) do
			local val = v.vert[d]
			if (val < min) then min = val end
			if (val > max) then max = val end
		end

		piece.mins[d] = min
		piece.maxs[d] = max
	end
end

local function calc_piece_offsets(piece, ox, oy, oz)
	ox = ox + piece.xoffset
	oy = oy + piece.yoffset
	oz = oz + piece.zoffset

	piece.ox = ox
	piece.oy = oy
	piece.oz = oz

	for _, child in ipairs(piece.childs) do
		calc_piece_offsets(child, ox, oy, oz)
	end
end



local function parse_vertex()
	local v = {
		vert = {
			[1] = read_float32(raw_model_data),
			[2] = read_float32(raw_model_data),
			[3] = read_float32(raw_model_data),
		},
		norm = {
			[1] = read_float32(raw_model_data),
			[2] = read_float32(raw_model_data),
			[3] = read_float32(raw_model_data),
		},
		txcd = {
			[1] = read_float32(raw_model_data),
			[2] = read_float32(raw_model_data),
		},
	}

	return v
end

local function parse_piece(pos, pieces)
	raw_model_data.index = pos

	local piece = {
		name            = read_uint32(raw_model_data),
		numChilds       = read_uint32(raw_model_data),
		childs          = read_uint32(raw_model_data),
		numVertices     = read_uint32(raw_model_data),
		vertices        = read_uint32(raw_model_data),
		vertexType      = read_uint32(raw_model_data),
		primitiveType   = read_uint32(raw_model_data),
		vertexTableSize = read_uint32(raw_model_data),
		vertexTable     = read_uint32(raw_model_data),
		collisionData   = read_uint32(raw_model_data),
		xoffset         = read_float32(raw_model_data),
		yoffset         = read_float32(raw_model_data),
		zoffset         = read_float32(raw_model_data),
	}

	pieces[#pieces + 1] = piece

	piece.id = #pieces
	piece.name = read_sub_string(raw_model_data, piece.name + 1)

	-- note: S3O's store the raw (global) position transform
	Spring.Echo("[parse_piece] " .. piece.name .. ", global offset: " .. piece.xoffset .. ", " .. piece.yoffset .. ", " .. piece.zoffset)


	-- vertex data
	local vertices = {}
	-- vertex index data
	local indices = {}
	-- child file offsets
	local childOffTbl = {}
	-- child pieces
	local childs = {}

	raw_model_data.index = piece.vertices + 1

	for v = 1, piece.numVertices do
		vertices[v] = parse_vertex()
	end


	raw_model_data.index = piece.vertexTable + 1

	for i = 1, piece.vertexTableSize do
		-- FIXME: index can be -1 for tristrips
		indices[i] = read_uint32(raw_model_data) + 1
	end


	raw_model_data.index = piece.childs + 1

	for child = 1, piece.numChilds do
		childOffTbl[child] = read_uint32(raw_model_data) + 1
	end

	for child = 1, piece.numChilds do
		childs[child] = parse_piece(childOffTbl[child], pieces)
		childs[child].parent = piece.id
	end

	-- replace the raw offsets by tables
	piece.vertices = vertices
	piece.vertexTable = indices
	piece.childs = childs

	set_piece_bounds(piece)

	return piece
end

local function parse_header_and_pieces()
	local pieces = {}
	local header = {
		magic         = read_string(raw_model_data, 12),
		version       = read_uint32(raw_model_data),
		radius        = read_float32(raw_model_data),
		height        = read_float32(raw_model_data),
		midx          = read_float32(raw_model_data),
		midy          = read_float32(raw_model_data),
		midz          = read_float32(raw_model_data),
		rootpiece     = read_uint32(raw_model_data),
		collisionData = read_uint32(raw_model_data),
		texture1      = read_uint32(raw_model_data),
		texture2      = read_uint32(raw_model_data),
	}

	header.rootpiece = parse_piece(header.rootpiece + 1, pieces)
	header.rootpiece.parent = 0

	header.texture1 = read_sub_string(raw_model_data, header.texture1 + 1)
	header.texture2 = read_sub_string(raw_model_data, header.texture2 + 1)

	return header, pieces
end




local function read_s3o_model(filename)
	raw_model_data.bytes = VFS.LoadFile(filename)
	raw_model_data.index = 1

	if (raw_model_data.bytes == nil) then
		return nil, nil
	end

	local header = nil
	local pieces = nil
	local pieceMap = {}
	local primTypes = {
		'triangles',
		'tristrip',
		'quads',
	}

	-- also parses the pieces
	header, pieces = parse_header_and_pieces()

	-- make the piece-map
	for pieceNum, piece in ipairs(pieces) do
		local oldPieceNum = pieceMap[piece.name]

		if (oldPieceNum == nil) then
			pieceMap[piece.name] = pieceNum
		else
			print(strfmt('Piece name conflict: %s (%i/%i)', piece.name, pieceNum, oldPieceNum))
		end
	end

	calc_piece_offsets(header.rootpiece, 0, 0, 0)

	for pieceNum = 1, #pieces do
		local p = pieces[pieceNum]

		print('piece #' .. pieceNum ..' = ' .. p.name)
		print('  parent          = ' .. p.parent)
		print('  numChilds       = ' .. p.numChilds)
		print('  primitiveType   = ' .. primTypes[p.primitiveType + 1])
		print('  numVertices     = ' .. p.numVertices)
		print('  vertexTableSize = ' .. p.vertexTableSize)
		print('  mins            = ' .. p.mins[1] .. ', ' .. p.mins[2] .. ', ' .. p.mins[3])
		print('  maxs            = ' .. p.maxs[1] .. ', ' .. p.maxs[2] .. ', ' .. p.maxs[3])
		print('  gmins           = ' .. (p.mins[1] + p.ox) .. ', ' .. (p.mins[2] + p.oy) .. ', ' .. (p.mins[3] + p.oz))
		print('  gmaxs           = ' .. (p.maxs[1] + p.ox) .. ', ' .. (p.maxs[2] + p.oy) .. ', ' .. (p.maxs[3] + p.oz))
	end

	print()
	print('------------------------------------------')
	print_piece_tree(header.rootpiece)
	print('------------------------------------------')

	print()
	print('filename = ' .. filename)
	print('length   = ' .. #raw_model_data.bytes)
	print('texture0 = ' .. header.texture1)
	print('texture1 = ' .. header.texture2)
	print()

	return header, pieces
end

local function save_obj_model(filename, header, pieces)
	if (header == nil or pieces == nil) then
		return
	end

	local f, err = io.open(filename, 'w')

	if (f == nil) then
		print(err)
		return
	end

	f:write('\n')
	f:write('# name: '     .. filename..'\n')
	f:write('# texture0: ' .. header.texture1..'\n')
	f:write('# texture1: ' .. header.texture2..'\n')


	local function get_prim_name(prim)
		if (prim == 0) then return 'triangles' end
		if (prim == 1) then return 'tristrips' end
		if (prim == 2) then return 'quads'     end
		return 'unknown'
	end

	local function write_vertices(vtable)
		local pvcount = 0

		for _, v in ipairs(vtable) do
			-- this adds the global piece-position to each vertex
			-- (thus written positions are in global model-space)
			-- f:write('v  ' .. (v.vert[1] + p.ox) .. ' ' .. (v.vert[2] + p.oy) .. ' ' ..(v.vert[3] + p.oz) .. '\n')
			f:write('v  ' .. (v.vert[1]       ) .. ' ' .. (v.vert[2]       ) .. ' ' ..(v.vert[3]       ) .. '\n')
			f:write('vn ' .. v.norm[1] .. ' ' .. v.norm[2] .. ' ' .. v.norm[3] .. '\n')
			f:write('vt ' .. v.txcd[1] .. ' ' .. v.txcd[2] .. '\n')
			pvcount = pvcount + 1
		end

		return pvcount
	end

	local function write_vertex_index(idx, pvcount)
		idx = idx - pvcount - 1
		f:write(' ' .. idx .. '/' .. idx .. '/' ..idx)
	end


	for i, p in ipairs(pieces) do
		local vtable = p.vertices
		local itable = p.vertexTable

		-- object info
		f:write('\n')
		-- f:write('o piece' .. i .. '\n')
		f:write('o ' .. p.name .. '\n')
		f:write('# name: ' .. p.name .. ', number: ' .. i .. '\n')

		f:write('# ' .. get_prim_name(p.primitiveType) .. ': count = ' .. #p.vertexTable .. '\n')
		f:write('# local  offset (wrt. parent): ' .. p.xoffset .. ' ' .. p.yoffset .. ' ' .. p.zoffset .. '\n')
		f:write('# global offset (wrt. root  ): ' .. p.ox .. ' ' .. p.oy .. ' ' .. p.oz .. '\n')
		f:write('\n')

		-- vertices
		local pvcount = write_vertices(vtable)

		-- faces
		if (p.primitiveType == 0) then -- triangles
			local itable = p.vertexTable

			for idx = 1, #itable, 3 do
				f:write('f ')
				write_vertex_index(itable[idx + 0], pvcount)
				write_vertex_index(itable[idx + 1], pvcount)
				write_vertex_index(itable[idx + 2], pvcount)
				f:write('\n')
			end

		elseif (p.primitiveType == 1) then -- tristrips
			f:write('f ')

			for idx = 1, (#itable - 2) do
				f:write('f ')
				if (math.abs(math.mod(idx, 2)) < 0.1) then
					write_vertex_index(itable[idx + 0], pvcount)
					write_vertex_index(itable[idx + 2], pvcount)
					write_vertex_index(itable[idx + 1], pvcount)
				else
					write_vertex_index(itable[idx + 0], pvcount)
					write_vertex_index(itable[idx + 1], pvcount)
					write_vertex_index(itable[idx + 2], pvcount)
				end
				f:write('\n')
			end

		elseif (p.primitiveType == 2) then -- quads
			local itable = p.vertexTable

			for idx = 1, #itable, 4 do
				f:write('f ')
				write_vertex_index(itable[idx + 0], pvcount)
				write_vertex_index(itable[idx + 1], pvcount)
				write_vertex_index(itable[idx + 2], pvcount)
				write_vertex_index(itable[idx + 3], pvcount)
				f:write('\n')
			end
		end
	end

	f:close()
end




function widget:GetInfo()
	return {
		name      = "model_s3o_to_obj_converter.lua",
		desc      = "converts S3O model files to OBJ format",
		date      = "Nov 1, 2009",
		license   = "GNU GPL, v2 or later",
		layer     = 1,
		enabled   = false
	}
end

function widget:Initialize()
	save_obj_model(dst_model_filename, read_s3o_model(src_model_filename))
end

