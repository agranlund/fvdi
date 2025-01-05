RVGA.SYS - Raven VGA driver for fVDI
Copyright (C) 2024 Anders Granlund

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

How to use:

1) Prepare your C: filesystem:
- Put FVDI.PRG in \AUTO folder
- Put RVGA.SYS in \GEMSYS folder
- Put the provided FVDI.SYS in the root folder

2) Select your resolution:
Edit FVDI.SYS with a text editor and uncomment one of the saga.sys lines at
bottom:
01r rvga.sys mode 640x480x8@60
The mode is in the form: WIDTHxHEIGHTxDEPTH@FREQ
WIDTH and HEIGHT are currently limited to the provided examples.
DEPTH must always be 8. FREQ is ignored.
