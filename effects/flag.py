import hyperion, time
hyperion.imageMinSize(10,10)

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
	# official country codes
	#############
	# EU flags -> missing: cyprus, UK
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
	
	# cz flag (Czech Republic)
	if country == "cz":
		hyperion.imageSolidFill(255, 255, 255)
		hyperion.imageSolidFill(0, int(iH*0.5), iW, iH, 215, 20, 26)
		hyperion.imageDrawPolygon(bytearray([0,0,int(iH*0.5),int(iH*0.5),0,iH]),17, 69, 126)
	
	# gr flag (Greece)
	if country == "gr":
		hyperion.imageSolidFill(13, 94, 175)
		hyperion.imageDrawLine(int(iW*0.35), int(iH*0.15), iW, int(iH*0.15),int(0.1*iH), 255, 255, 255)
		hyperion.imageDrawLine(int(iW*0.35), int(iH*0.35), iW, int(iH*0.35),int(0.1*iH), 255, 255, 255)
		hyperion.imageDrawLine(0, int(iH*0.55), iW, int(iH*0.55),int(0.1*iH), 255, 255, 255)
		hyperion.imageDrawLine(0, int(iH*0.75), iW, int(iH*0.75),int(0.1*iH), 255, 255, 255)
		hyperion.imageDrawLine(int(0.175*iW), 0, int(0.175*iW), int(iH*0.35),int(0.1*iH), 255, 255, 255)
		hyperion.imageDrawLine(0, int(0.175*iW), int(iH*0.35), int(0.175*iW),int(0.1*iH), 255, 255, 255)
		
	#############
	# other flags
	#############

	# ch flag (Swiss)
	if country == "ch":
		hyperion.imageSolidFill(255, 0, 0)
		hyperion.imageDrawLine(int(iW*0.5), int(iH*0.25), int(iW*0.5), int(iH*0.75), int(iH*0.15), 255, 255, 255)
		hyperion.imageDrawLine(int(iW*0.26), int(iH*0.5), int(iW*0.75), int(iH*0.5), int(iH*0.15), 255, 255, 255)
		
	# cmr flag (Cameroon)
	if country == "cmr":
		hyperion.imageSolidFill(0, 0, int(iW*0.33), iH, 0, 255, 0)
		hyperion.imageSolidFill(int(iW*0.33), 0, int(iW*0.33), iH, 255, 0, 0)
		hyperion.imageSolidFill(int(iW*0.66), 0, iW, iH, 255, 255, 0)
		
	# ru flag (Russia)
	if country == "ru":
		hyperion.imageSolidFill(0, 0, iW, int(iH*0.33), 255, 255, 255)
		hyperion.imageSolidFill(0, int(iH*0.33), iW, int(iH*0.33), 0, 57, 166)
		hyperion.imageSolidFill(0, int(iH*0.66), iW, iH, 213, 43, 30)

	# gb-eng flag (England)
	if country == "gb-eng":
		hyperion.imageSolidFill(255, 255, 255)
		hyperion.imageDrawLine(0, int(iH*0.50), iW, int(iH*0.50), int(iW*0.10), 255, 0, 0)
		hyperion.imageDrawLine(int(iW*0.50), 0, int(iW*0.50), iH, int(iW*0.10), 255, 0, 0)

	# gb-sct flag (Scotland)
	if country == "gb-sct":
		hyperion.imageSolidFill(0, 0, 255)
		hyperion.imageDrawLine(0, 0, iW, iH, int(iW*0.15), 255, 255, 255)
		hyperion.imageDrawLine(0, iH, iW, 0, int(iW*0.15), 255, 255, 255)                                        

	# gb flag (United Kingdom)
	if country == "gb":
		hyperion.imageSolidFill(0, 0, 255)
		hyperion.imageDrawLine(0, int(iH*0.50), iW, int(iH*0.50), int(iW*0.25), 255, 255, 255)
		hyperion.imageDrawLine(int(iW*0.50), 0, int(iW*0.50), iH, int(iW*0.25), 255, 255, 255)
		hyperion.imageDrawLine(0, 0, iW, iH, int(iH*0.25), 255, 255, 255)
		hyperion.imageDrawLine(0, iH, iW, 0, int(iH*0.25), 255, 255, 255)
		hyperion.imageDrawLine(0, int(iH*0.50), iW, int(iH*0.50), int(iW*0.15), 255, 0, 0)
		hyperion.imageDrawLine(int(iW*0.50), 0, int(iW*0.50), iH, int(iW*0.15), 255, 0, 0)
		hyperion.imageDrawLine(int(iW*0.95), 0, int(iW*0.475), int(iH*0.50), int(iH*0.10), 255, 0, 0)
		hyperion.imageDrawLine(int(iW*0.05), iH, int(iW*0.525), int(iH*0.50), int(iH*0.10), 255, 0, 0)
		hyperion.imageDrawLine(int(-iW*0.05), 0, int(iW*0.475), int(iH*0.50), int(iH*0.10), 255, 0, 0)
		hyperion.imageDrawLine(int(iW*1.05), iH, int(iW*0.475), int(iH*0.50), int(iH*0.10), 255, 0, 0)

#prepare wanted flags
for cf in countries:
	printFlag(cf)
	imgIds.append(hyperion.imageSave())
	hyperion.imageSolidFill(0, 0, 0)

while not hyperion.abort():
	switchImage()
	time.sleep(duration)
