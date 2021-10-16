on run argv
    set image_name to item 1 of argv
    tell application "Finder"
        tell disk image_name

            -- wait for the image to finish mounting
            set open_attempts to 0
            repeat while open_attempts < 4
                try
                    open
                        delay 1
                        set open_attempts to 5
                    close
                    on error errStr number errorNumber
                        set open_attempts to open_attempts + 1
                        delay 10
                end try
            end repeat
            delay 5

            -- open the image the first time and save a DS_Store with just
            -- background and icon setup
            open
                set current view of container window to icon view
                set theViewOptions to the icon view options of container window
                set background picture of theViewOptions to file ".background:background.png"
                set arrangement of theViewOptions to not arranged
                set icon size of theViewOptions to 100
                delay 5
            close

            -- next setup the position of the app and Applications symlink
            -- plus hide all the window decoration
            open
                tell container window
                    set sidebar width to 0
                    set toolbar visible to false
                    set statusbar visible to false
                    set the bounds to {300, 100, 1000, 548}
                    set position of item "Hyperion.app" to {260, 230}
                    set extension hidden of item "Hyperion.app" to true
                    set position of item "Applications" to {590, 228}

                    -- Move these out of the way for users with Finder configured to show all files
                    set position of item ".background" to {800, 280}
                    set position of item ".fseventsd" to {800, 280}
                    set position of item ".VolumeIcon.icns" to {800, 280}
                end tell
                delay 1
            close

            -- one last open and close so you can see everything looks correct
            open
                delay 5
            close

        end tell

        delay 1
    end tell
end run
