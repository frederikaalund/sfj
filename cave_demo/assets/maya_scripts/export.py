import maya.cmds
import os

mayaPath = maya.cmds.file(query=True, sceneName=True)
fileName = os.path.splitext(mayaPath)[0]

maya.cmds.file(fileName + ".fbx", exportAll=True, type="FBX export", force=True, preserveReferences=True)
maya.cmds.file(fileName + ".bullet", exportAll=True, type="Bullet Physics export", force=True, preserveReferences=True)