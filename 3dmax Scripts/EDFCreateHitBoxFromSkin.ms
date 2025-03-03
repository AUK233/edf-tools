struct EDFCreateHitBoxFromSkin
(
	fn RotatePivotOnly obj rotation = (
		local rotValInv=inverse (rotation as quat)
		animate off in coordsys local obj.rotation*=RotValInv
		obj.objectoffsetrot*=RotValInv
		obj.objectoffsetpos*=RotValInv
	),

	fn createBoneInfluencedMesh obj = (
		local skinMod = obj.modifiers[#Skin]
		if skinMod == undefined do return

		-- Select Object
		max modify mode
		-- select obj
		modPanel.setCurrentObject skinMod node:obj

		-- Save new mesh
		local hktMeshDict = #()

		for i = 1 to (skinOps.GetNumberBones skinMod) do (
			local boneName = skinOps.GetBoneName skinMod i 0
			local boneNode = getNodeByName boneName
			if boneNode == undefined do continue

			local vertsCount = 0
			local vertsToDelete = #()

			-- Get vertices effected by current bone
			for v = 1 to (getNumVerts obj) do (
				local weightCount = skinOps.GetVertexWeightCount skinMod v
				for w = 1 to weightCount do (
					local boneId = skinOps.GetVertexWeightBoneID skinMod v w
					local weight = skinOps.GetVertexWeight skinMod v w
					if boneId == i and weight > 0.95 then (
						vertsCount += 1;
						exit -- like c break
					) else (
						append vertsToDelete v
						exit
					)
				)
			)

			if vertsCount > 0 do (
				local newMesh = copy obj
				newMesh.name = "hkt." + boneName

				-- Remove useless
				meshop.deleteVerts newMesh vertsToDelete
				for m = newMesh.modifiers.count to 1 by -1 do (
					if classOf newMesh.modifiers[m] == Skin do (
						deleteModifier newMesh m
					)
				)

				-- Set axis to be the same as the bone
				--newMesh.pivot = boneNode.transform.translationpart
				newMesh.pivot = boneNode.pos
				RotatePivotOnly newMesh boneNode.rotation 

				-- Set parent
				local boneParent = boneNode.parent
				if boneParent != undefined do (
					local parentHktMeshName = "hkt." + boneParent.name
					local parentHktMesh = getNodeByName parentHktMeshName

					if parentHktMesh != undefined do (
						newMesh.parent = parentHktMesh
					)
				)

				append hktMeshDict newMesh
			)
		)
	),

	fn CreateHitBox=
	(	
		--Output = newScript()

		if selection.count > 0 then
		(
			local selectionObject = #()
			for i = 1 to selection.count do
			(
				if ClassOf selection[i].baseObject == Editable_mesh then
				(
					append selectionObject selection[i]
				)
			)

			if selectionObject.count > 0 then (
				max modify mode
				for i = 1 to selectionObject.count do
				(
					createBoneInfluencedMesh selectionObject[i]
				)
			)
			--messagebox "Complete Hitbox generation." title:"Success"
		) -- end if
		else
		(
			messagebox "Error: No Objects Selected." title:"Error"
		)
	)
)