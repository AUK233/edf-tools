Export the model for MDB:

-------------------------

1.Import MDB:
Drag and drop MDB to "EDF Tools.exe", you will get the file you can edit.

Now this xml can be imported using a script.

If the imported model has skins, You need to open the weight table and use the "Clear Zero Weights" button to clear all useless zero weights.
    Note that you have to clear the 0 weights, otherwise you will have problems exporting.

-------------------------

2.Export MDB

2.1.Check Vertices:
    You can check the vertex count manually.
    Or select a model to use this command:
        selection[1].numverts

    ok, you have to select a model then use this command:
        selection[1].numtverts

    If the two are not equal in number, you should increase the modifier "Unwrap UVs".
    You should collapse this modifier, taking care not to collapse the skin as well.

    If they are still not equal, or the exported model textures display abnormally.
    Select this model and do "Fix vertex UVs", it works fine to a certain extent.

2.2.To MDB:
    First export the model to a text.

    Then copy the contents of ObjectLists and replace the contents of *_MDB.xml.
        Note that only models can be exported for now!

    If your model uses a different material, you need to change 0 in Mesh MatID="0" to corresponding.

    Delete the content of Names and Textures, you can not delete them, but there may be problems.

    Drag and drop *_MDB.xml to "EDF Tools.exe". If executed correctly, you will get an MDB file.
        Note that the filename must have "_MDB" at the end, otherwise the tool will not be able to parse it.