Charles Pastuszenski
Computer Graphics
CS77
Assignment 4 README


ABOUT:
	
	In Assignment 4, students implemented an advanced raytracing suite.
	
	The functionality I implemented in my assignment was:
		Texture mapping 
		Texture filtering 
		Depth of Field with Disk Sampling
		Distribution Raytracing - Soft Shadows
		Ambient Occlusion

	The large rendered image can be found in this directory; it's called big_render.png. 
	It looks like something that could be on the cover of a computer graphics book!!

BUILD NOTES:

	To build the project, simply type "make" in a terminal from the directory in which this
	README is located. To run the test program, type "sh render_all.sh" in the scenes directory
	to run the shell script I wrote that performs before/after render tests for each of the features.
	
	The images produced will be:
		- cornell_soft.png - Cornell box with soft shadows
		- cornell_hard.png - Cornell box with hard shadows
		- DOF_disk_test.png - Rendering of two spheres using DOF/disk sampling
		- DOF_test.png - Rendering of two spheres with DOF only
		- noDOF_test.png - Rendering of two spheres with no DOF
		- robot_with_occlusion.png - Robot rendering with ambient occlusion	
		- robot_no_occlusion.png - Robot rendering without ambient occlusion
		- texture_mapping_test.png - Texture mapping test
		- texture_mapping_test_no_textures.png - Texture mapping test with no textures
		- trilinear_test.png - Texture filtering test
		- trilinear_test_no_filtering.png - Same as previous test, without trilinear filtering
