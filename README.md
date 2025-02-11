PICSL Image Graph Cut Tool
==========================

This project provides a CLI tool and a Python interface for taking a binary 3D volume (e.g., in NIFTI format or any other readable by ITK) and splitting it into components based on connectivity. The code is a wrapper around the METIS library. 

Quick Start (Python)
--------------------
Install the package:

```sh
pip install picsl_image_graph_cut
```

Download a binary image to experiment with

```sh
DATAURL=https://github.com/pyushkevich/greedy/raw/master/testing/data
curl -L $DATAURL/phantom01_mask.nii.gz -o phantom01_mask.nii.gz
```

Partition the image 

```python
from picsl_image_graph_cut import image_graph_cut
help(image_graph_cut)
image_graph_cut('phantom01_mask.nii.gz', 'phantom01_gcut.nii.gz', 5)
```

