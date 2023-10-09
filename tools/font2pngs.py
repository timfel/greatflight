import os
import sys
from PIL import Image

i = Image.open(sys.argv[1])
os.makedirs(os.path.dirname(sys.argv[2]), exist_ok=True)
