An implementation of Catmull-Clark Subdivision algorithm on a 3D object. Up to three levels of subdivision are implemented. The 3D-I's meshes are shown and its sharp edges will be made smooth and rounded by the algorithm. In addition, texture is mapped on the object. 

Before compiling, please install the newest version of SOIL library by running the command:

sudo apt-get install libsoil-dev

The libsoil should then be installed to your /usr/include/ directory; you can find it by running the command:

dpkg -L libsoil-dev | grep include

then if it's successfully installed, it should show:
/usr/include
/usr/include/SOIL
/usr/include/SOIL/SOIL.h
/usr/include/SOIL/image_DXT.h
/usr/include/SOIL/image_helper.h
/usr/include/SOIL/stbi_DDS_aug.h
/usr/include/SOIL/stbi_DDS_aug_c.h
/usr/include/SOIL/stb_image_aug.h

You also should have OpenGL Mathematics (GLM) library installed.

This program performs Catmull-Clark subdivisions up tp three levels on a 3D letter I . The camera position follows a cubic Besier path. The letter-I can also be viewed with some textures.

To run the code, compile by typing "make", press enter.

After compiling, run the code by typing "./mhu9_mp4", press enter.

An 3D letter-I with no subdivision will show up and the camera keeps moving along a cubic Bezier path. Note that it seems that the I is moving itself, but it is actually the camera that is moving along a cubic Bezier curve path. To verify this, you can check the function call "getCurrentEyePosition()" at line 832 which updates the eye position using a time parameter 0-1 and the 4 base functions (see lines 668-696).

Please press the following keys to increase the subdivision level and to perform other functionality:

Press 'W' or 'UP' arrow key to increase the subdivision level.
Press 'S' or 'DOWN' arrow key to decrease the subdivision level. 
Press 'L' to change the lookat point from the origin to one of the I's vertex, press it again to change back.
Press 'T' to show/unshow the I's textures.
NOTE: the texture is a multitexture with texture and environmental mapping.
Press 'G' to turn on/off the light source.
Press 'A' to stop the camera from moving. Press 'A' again to resume camera's movement.

Press 'Esc' to quit the program.
