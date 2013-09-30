#!/bin/bash
#Run this from scenes directory

function check_error {
    if [ "$?" -ne "0" ]; then
        cd ..
        echo "An error occurred!"
        exit 1
    fi
}

echo "Texture Filtering (After)"
time ../trace scene_trilinear_test.json trilinear_test.png
check_error

echo "Texture Filtering (Before)"
time ../trace scene_trilinear_test_no_filtering.json trilinear_test_no_filtering.png
check_error

echo "Texture Mapping (After)"
time ../trace scene_textures.json texture_mapping_test.png
check_error

echo "Texture Mapping (Before)"
time ../trace scene_textures_no_texturing.json texture_mapping_test_no_textures.png
check_error

echo "Ambient Occlusion (After)"
time ../trace -s 256 -d scene_robot_occlusion.json robot_with_occlusion.png
check_error

echo "Ambient Occlusion (Before)"
time ../trace -s 256 -d scene_robot.json robot_no_occlusion.png
check_error

echo "Distribution Raytracing - Soft Shadows (After)"
time ../trace -s 128 -d scene_cornellbox.json cornell_soft.png
check_error

echo "Cornell Box Normally Raytraced - Distribution Raytracing - Soft Shadows (Before)"
time ../trace scene_cornellbox.json cornell_hard.png
check_error

echo "Depth of Field"
time ../trace -s 128 -d scene_focus_DOF.json DOF_test.png
check_error

echo "Depth of Field with Disk Sampling"
time ../trace -s 128 -d scene_focus_DOF_disk.json DOF_disk_test.png
check_error

echo "Depth of Field Scene with no DOF/Disk Sampling"
time ../trace -s 128 -d scene_focus_noDOF.json noDOF_test.png
check_error

echo "All completed successfully!"

cd ..
