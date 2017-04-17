import hyperion, time
#hyperion.imageMinSize(32,32)

iW = hyperion.imageWidth()
iH = hyperion.imageHeight()
countries  = hyperion.args.get('countries', ("de","at"))
duration  = int(hyperion.args.get('switch-time', 5))
imgIds = []
nr = 0

def switchImage():
	global nr
	if nr >= len(imgIds):
		nr = 0
	hyperion.imageShow(imgIds[nr])
	nr += 1

def printFlag(country):
	# offcial country codes
	#############
	# EU flags -> missing cyprus, Czech Republic, Greece, UK
	#############
	# de flag (Germany)
	if country == "de":
		hyperion.imageSolidFill(0, 0, iW, int(iH*0.33), 0, 0, 0)
		hyperion.imageSolidFill(0, int(iH*0.33), iW, int(iH*0.33), 221, 0, 0)
		hyperion.imageSolidFill(0, int(iH*0.66), iW, iH, 255, 206, 0)

	# at flag (Austria)
	if country == "at":
		hyperion.imageSolidFill(237, 41, 57)
		hyperion.imageSolidFill(0, int(iH*0.33), iW, int(iH*0.33), 255, 255, 255)

	# fr flag (France)
	if country == "fr":
		hyperion.imageSolidFill(0, 0, int(iW*0.33), iH, 0, 35, 149)
		hyperion.imageSolidFill(int(iW*0.33), 0, int(iW*0.33), iH, 255, 255, 255)
		hyperion.imageSolidFill(int(iW*0.66), 0, iW, iH, 237, 41, 57)

	# be flag (Belgium)
	if country == "be":
		hyperion.imageSolidFill(0, 0, int(iW*0.33), iH, 0, 0, 0)
		hyperion.imageSolidFill(int(iW*0.33), 0, int(iW*0.33), iH, 255, 224, 66)
		hyperion.imageSolidFill(int(iW*0.66), 0, iW, iH, 237, 41, 57)

	# it flag (Italy)
	if country == "it":
		hyperion.imageSolidFill(0, 0, int(iW*0.33), iH, 0, 146, 70)
		hyperion.imageSolidFill(int(iW*0.33), 0, int(iW*0.33), iH, 255, 255, 255)
		hyperion.imageSolidFill(int(iW*0.66), 0, iW, iH, 206, 43, 55)

	# es flag (Spain)
	if country == "es":
		hyperion.imageSolidFill(198, 11, 30)
		hyperion.imageSolidFill(0, int(iH*0.25), iW, int(iH*0.55), 255, 196, 0)
		
	# bg flag (Bulgaria)
	if country == "bg":
		hyperion.imageSolidFill(0, 0, iW, int(iH*0.33), 255, 255, 255)
		hyperion.imageSolidFill(0, int(iH*0.33), iW, int(iH*0.33), 0, 150, 110)
		hyperion.imageSolidFill(0, int(iH*0.66), iW, iH, 214, 38, 18)

	# ee flag (Estonia)
	if country == "ee":
		hyperion.imageSolidFill(0, 0, iW, int(iH*0.33), 72, 145, 217)
		hyperion.imageSolidFill(0, int(iH*0.33), iW, int(iH*0.33), 0, 0, 0)
		hyperion.imageSolidFill(0, int(iH*0.66), iW, iH, 255, 255, 255)

	# dk flag (Denmark)
	if country == "dk":
		hyperion.imageSolidFill(198, 12, 48)
		hyperion.imageDrawLine(int(iW*0.35), 0, int(iW*0.35), iH, int(iW*0.13), 255, 255, 255)
		hyperion.imageDrawLine(0, int(iH*0.5), iW, int(iH*0.5), int(iW*0.13), 255, 255, 255)

	# fi flag (Finland)
	if country == "fi":
		hyperion.imageSolidFill(255, 255, 255)
		hyperion.imageDrawLine(int(iW*0.35), 0, int(iW*0.35), iH, int(iW*0.18), 0, 53, 128)
		hyperion.imageDrawLine(0, int(iH*0.5), iW, int(iH*0.5), int(iW*0.18), 0, 53, 128)

	# hu flag (Hungary)
	if country == "hu":
		hyperion.imageSolidFill(0, 0, iW, int(iH*0.33), 205, 42, 62)
		hyperion.imageSolidFill(0, int(iH*0.33), iW, int(iH*0.33), 255, 255, 255)
		hyperion.imageSolidFill(0, int(iH*0.66), iW, iH, 67, 111, 77)

	# ie flag (Ireland)
	if country == "ie":
		hyperion.imageSolidFill(0, 0, int(iW*0.33), iH, 0, 155, 72)
		hyperion.imageSolidFill(int(iW*0.33), 0, int(iW*0.33), iH, 255, 255, 255)
		hyperion.imageSolidFill(int(iW*0.66), 0, iW, iH, 255, 121, 0)

	# lv flag (Latvia)
	if country == "lv":
		hyperion.imageSolidFill(158, 48, 57)
		hyperion.imageDrawLine(0, int(iH*0.5), iW, int(iH*0.5), int(iH*0.2), 255, 255, 255)

	# lt flag (Lithuanian)
	if country == "lt":
		hyperion.imageSolidFill(0, 0, iW, int(iH*0.33), 253, 185, 19)
		hyperion.imageSolidFill(0, int(iH*0.33), iW, int(iH*0.33), 0, 106, 68)
		hyperion.imageSolidFill(0, int(iH*0.66), iW, iH, 193, 39, 45)

	# lu flag (Luxembourg)
	if country == "lu":
		hyperion.imageSolidFill(0, 0, iW, int(iH*0.33), 237, 41, 57)
		hyperion.imageSolidFill(0, int(iH*0.33), iW, int(iH*0.33), 255, 255, 255)
		hyperion.imageSolidFill(0, int(iH*0.66), iW, iH, 0, 161, 222)

	# mt flag (Malta)
	if country == "mt":
		hyperion.imageSolidFill(0, 0, int(iW*0.5), iH, 255, 255, 255)
		hyperion.imageSolidFill(int(iW*0.5), 0, int(iW*0.5), iH, 207, 20, 43)

	# nl flag (Netherlands)
	if country == "nl":
		hyperion.imageSolidFill(0, 0, iW, int(iH*0.33), 174, 28, 40)
		hyperion.imageSolidFill(0, int(iH*0.33), iW, int(iH*0.33), 255, 255, 255)
		hyperion.imageSolidFill(0, int(iH*0.66), iW, iH, 33, 70, 139)
		
	# pl flag (Poland)
	if country == "pl":
		hyperion.imageSolidFill(255, 255, 255)
		hyperion.imageSolidFill(0, int(iH*0.5), iW, iH, 220, 20, 60)

	# pt flag (Portugal)
	if country == "pt":
		hyperion.imageSolidFill(0, 102, 0)
		hyperion.imageSolidFill(int(iW*0.4), 0, iW, iH, 255, 0, 0)

	# ro flag (Romania)
	if country == "ro":
		hyperion.imageSolidFill(0, 0, int(iW*0.33), iH, 0, 43, 127)
		hyperion.imageSolidFill(int(iW*0.33), 0, int(iW*0.33), iH, 252, 209, 22)
		hyperion.imageSolidFill(int(iW*0.66), 0, iW, iH, 206, 17, 38)

	# sl flag (Slovakia/Slovenia)
	if country == "sl":
		hyperion.imageSolidFill(0, 0, iW, int(iH*0.33), 255, 255, 255)
		hyperion.imageSolidFill(0, int(iH*0.33), iW, int(iH*0.33), 11, 78, 162)
		hyperion.imageSolidFill(0, int(iH*0.66), iW, iH, 238, 28, 37)

	# se flag (Sweden)
	if country == "se":
		hyperion.imageSolidFill(0, 106, 167)
		hyperion.imageDrawLine(int(iW*0.35), 0, int(iW*0.35), iH, int(iW*0.14), 254, 204, 0)
		hyperion.imageDrawLine(0, int(iH*0.5), iW, int(iH*0.5), int(iW*0.14), 254, 204, 0)

	#############
	# other flags
	#############

	# ch flag (Swiss)
	if country == "ch":
		hyperion.imageSolidFill(255, 0, 0)
		hyperion.imageDrawLine(int(iW*0.5), int(iH*0.25), int(iW*0.5), int(iH*0.75), int(iH*0.15), 255, 255, 255)
		hyperion.imageDrawLine(int(iW*0.26), int(iH*0.5), int(iW*0.75), int(iH*0.5), int(iH*0.15), 255, 255, 255)

#prepare wanted flags
for cf in countries:
	printFlag(cf)
	imgIds.append(hyperion.imageSave())
	hyperion.imageSolidFill(0, 0, 0)

while not hyperion.abort():
	switchImage()
	time.sleep(duration)
