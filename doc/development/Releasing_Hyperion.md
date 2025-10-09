# Build a new Release

## Preparation

- Check, if new Ubuntu or Debian versions need to be added as build environment or remove unsupported (optional)

- Merge all outstading PRs or changes valid to be included in the release. Address any github-code-scanning findings before.

- Update missing non-English translations in Poeditor (optional)

- Export translations provided since last release from Poeditor into Hyperion-Git

- Update the `.version` file with the new release version

- Update the `CHANGELOG.md` with missing documentation and change from "Unreleased" to new release version.

- Push updated `.version` & `CHANGELOG.md` to master or create an PR (in case you might want to add some minor, late fixes)

## Execution
 
- Push a new tag to the master branch of hyperion-project/hyperion.ng repository, e.g. `git push origin 2.0.15`
The push will create a draft release including an update to Hyperion's apt repository

- On Hyperion's apt repository, 
	- Backup the main directory, in case a fall back is requried (optional)
	- Move the content of the `draft-release` directory into the main diectory

- On GitHub, edit the draft release's description and publish the release
(this triggers the HyperBian build on top of the release)

- Check the HyperBian is build sucessfully with the correct release

## Rollover

Prepare next beta release and nighly builds

- Update the `.version` file with the next release version incl. beta.1, e.g. `2.0.16-beta.1`

- Add an  "Unreleased" selection to `CHANGELOG.md`, plus empty sections to allow capturing changes.

- Push updated `.version` & `CHANGELOG.md` to master

