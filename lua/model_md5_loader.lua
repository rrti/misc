function widget:GetInfo()
	return {
		name      = "model_md5_loader.lua",
		desc      = "loads and draws MD5 models",
		date      = "28-7-2008",
		license   = "GPL, v2",
		layer     = 0,
		enabled   = false,
	}
end

local mesh_name = "blob.md5mesh"
local mesh_text = "blob.jpg"

local joints = {}
local meshes = {}

function widget:Initialize()
	io.input(mesh_name)
	local total = io.read("*all")

	_, _, skel = string.find(total, "joints %{(.-)%}")

	local i ,j, t = 0, 0, -1
	local i2, j2, t2 = 0, 0, 0

	-- read joints
	while true do
		i, j, nr = string.find(skel, '(".-".-)\n', j + 1)

		if i == nil then
			break
		end

		t = t+1

		local _, _, name   = string.find(nr, '"(.-)"')
		local _, _, parent = string.find(nr, '".-" (.-) ')
		local _, _, x      = string.find(nr, '%( (.-) ')
		local _, _, y      = string.find(nr, '%( .- (.-) ')
		local _, _, z      = string.find(nr, '%( .- .- (.-) ')
		local _, _, xo     = string.find(nr, '%(.-%) %( (.-) ')
		local _, _, yo     = string.find(nr, '%(.-%) %( .- (.-) ')
		local _, _, zo     = string.find(nr, '%(.-%) %( .- .- (.-) ')

		local joint = {
			name = name,
			parent = tonumber(parent),

			x  = tonumber(x),
			y  = tonumber(y),
			z  = tonumber(z),
			xo = tonumber(xo),
			yo = tonumber(yo),
			zo = tonumber(zo),
		}

		local kuk = 1 - xo^2 - yo^2 - zo^2

		if kuk < 0 then
			joint.wo = 0
		else
			joint.wo = -math.sqrt(kuk)
		end

		joints[t] = joint   
	end

	-- read mesh
	while true do
		i2, j2, s = string.find(total, "mesh %{(.-)%}", j2 + 1)

		if i2 == nil then
			break
		end

		local verts   = {}
		local tris    = {}
		local weights = {}

		i, j = 0, 0

		-- read vertices
		while true do
			i, j, nr = string.find(s, "vert (.-)\n", i + 1)

			if i == nil then
				break
			end

			local _, _, t     = string.find(nr, "(%d+)")
			local _, _, u     = string.find(nr, "%( (.-) ")
			local _, _, v     = string.find(nr, "%( .- (.-) ")
			local _, _, start = string.find(nr, "%) (%d+)")
			local _, _, count = string.find(nr, "%) %d+ (%d+)")

			local vert = {
				u     = tonumber(u),
				v     = tonumber(v),
				start = tonumber(start),
				count = tonumber(count),
			}

			verts[tonumber(t)] = vert
		end

		i, j = 0, 0

		-- read geometry (triangles)
		while true do
			i, j, nr = string.find(s, "tri (.-)\n", i + 1)

			if i == nil then
				break
			end

			local _, _, t  = string.find(nr, "(%d+)")
			local _, _, v1 = string.find(nr, "%d+ (%d+)")
			local _, _, v2 = string.find(nr, "%d+ %d+ (%d+)")
			local _, _, v3 = string.find(nr, "%d+ %d+ %d+ (%d+)")

			local tri = {
				[1] = tonumber(v1),
				[2] = tonumber(v2),
				[3] = tonumber(v3),
			}

			tris[tonumber(t)] = tri   
		end

		i, j = 0, 0

		-- read joint weights
		while true do
			i, j, nr = string.find(s, "weight (.-)\n", i + 1)

			if i == nil then
				break
			end

			local _, _, t     = string.find(nr, "(%d+)")
			local _, _, joint = string.find(nr, "%d+ (%d+)")
			local _, _, bias  = string.find(nr, "%d+ %d+ (.-) ")
			local _, _, x     = string.find(nr, "%( (.-) ")
			local _, _, y     = string.find(nr, "%( .- (.-) ")
			local _, _, z     = string.find(nr, "%( .- .- (.-) ")

			local weight = {
				joint = tonumber(joint),
				bias  = tonumber(bias),
				x     = tonumber(x),
				y     = tonumber(y),
				z     = tonumber(z),
			}

			weights[tonumber(t)] = weight   
		end

		meshes[t2] = {
			verts   = verts,
			tris    = tris,
			weights = weights,
		}

		t2 = t2 + 1
	end 
end



-- multiply two quaternions
function qmult(qax, qay, qaz, qaw,  qbx, qby, qbz, qbw)
	local rw = (qaw * qbw) - (qax * qbx) - (qay * qby) - (qaz * qbz)
	local rx = (qax * qbw) + (qaw * qbx) + (qay * qbz) - (qaz * qby)
	local ry = (qay * qbw) + (qaw * qby) + (qaz * qbx) - (qax * qbz)
	local rz = (qaz * qbw) + (qaw * qbz) + (qax * qby) - (qay * qbx)
	return rx, ry, rz, rw
end

function qrot(qax, qay, qaz, qaw,  qbx, qby, qbz, qbw)
   rx, ry, rz, rw = qmult(qax, qay, qaz, qaw,  qbx, qby, qbz, qbw)
   rx, ry, rz, rw = qmult(rx, ry, rz, rw,  -qax, -qay, -qaz, qaw)
   return rx, ry, rz
end

function widget:DrawWorld()
	for _, mesh in pairs(meshes) do
		for _, weight in pairs(mesh.weights) do
			weight.xp, weight.yp, weight.zp = qrot(joints[weight.joint].xo, joints[weight.joint].yo, joints[weight.joint].zo, joints[weight.joint].wo,  weight.x, weight.y, weight.z, 0)
			weight.xp = weight.xp + joints[weight.joint].x
			weight.yp = weight.yp + joints[weight.joint].y
			weight.zp = weight.zp + joints[weight.joint].z
		end

		for _, vertex in pairs(mesh.verts) do
			vertex.xp, vertex.yp, vertex.zp = 0, 0, 0

			for i = 0, vertex.count - 1 do
				local bias = mesh.weights[vertex.start + i].bias
				vertex.xp = vertex.xp + mesh.weights[vertex.start + i].xp * bias
				vertex.yp = vertex.yp + mesh.weights[vertex.start + i].yp * bias
				vertex.zp = vertex.zp + mesh.weights[vertex.start + i].zp * bias
			end
		end

		gl.DepthTest(true)
		gl.DepthMask(true)

		gl.PushMatrix()
		gl.Translate(100.0, 100.0, 100.0)

		gl.Texture(mesh_text)
   
		for _, tri in pairs(mesh.tris) do
			gl.BeginEnd(GL.TRIANGLES, function()
				gl.TexCoord(0, 0)  gl.Vertex(mesh.verts[ tri[1] ].xp, mesh.verts[ tri[1] ].yp, mesh.verts[ tri[1] ].zp)
				gl.TexCoord(1, 0)  gl.Vertex(mesh.verts[ tri[2] ].xp, mesh.verts[ tri[2] ].yp, mesh.verts[ tri[2] ].zp)
				gl.TexCoord(1, 1)  gl.Vertex(mesh.verts[ tri[3] ].xp, mesh.verts[ tri[3] ].yp, mesh.verts[ tri[3] ].zp)
			end)
		end

		gl.PopMatrix();
	 end
end

