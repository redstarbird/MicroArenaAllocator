# Extracts the changes for a specific release from the changelog - part of release workflow
import os
import sys

def main():
    # Get version tag (without leading 'v')
    version = os.environ.get("GITHUB_REF_NAME", '').lstrip('v');

    # Ensure version
    if len(version) == 0:
        print("Error: Could not determine version from GITHUB_REF_NAME.")
        sys.exit(1)

    changelog_path = "CHANGELOG.md"
    output_path = "release_notes.md"

    # Ensure changelog file
    if not os.path.exists(changelog_path):
        print("Error:" + changelog_path + " changelog file cannot be found.")
        sys.exit(1)

    with open(changelog_path, 'r') as file:
        lines = file.readlines()

    release_notes = []
    is_reading = False
    target_header = f"## [{version}]"

    # Main parsing loop
    for line in lines:
        # If currently reading and a new header is found, the previous header must have ended
        if is_reading and line.startswith("## ["):
            break

        if is_reading:
            release_notes.append(line)
        
        if line.startswith(target_header):
            is_reading = True

    # If is_reading is false, the release notes were never found so raise error
    if not is_reading:
        print("Error: Release notes for version " + version + " not found!")
        sys.exit(1)

    # Put the release notes into the output file
    with open(output_path, 'w') as file:
        file.write(''.join(release_notes))

    print("Created release notes")
    
if __name__ == "__main__":
    main()