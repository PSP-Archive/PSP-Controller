

		PSP Controller v0.2


I needed a very simple way to use the PSP as a controller
for PC for a small game prototype I wanted to create. Just
to get the feeling of the controls. The PSP is not much good
as a controller, but it is very nice to test what is reachable
by the anatomy of your thumb in advance! 

It's nothing fancy, it just sends the status of the PSP
pad at 60Hz to the host PC (or what ever is listening to it)
via UDP. The source is very heavily based on the pspet wifi
test v0.3. Includes both, the source for hte PSP to send the
stuff and a small PC test app to receive the data.

If you find this useful, have fun!

Version 0.2
- converted the test application to a driver for PPJoy
  http://www.geocities.com/deonvdw/PPJoy.htm

Version 0.1
- initial release


	--mikko <memon@inside.org>