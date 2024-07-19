# Artifacts Reproducibility

[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.10670270.svg)](https://doi.org/10.5281/zenodo.12700677)


Below, we des^cribe how to reproduce the experiments in the Paper entitled:
"I/O Behind the Scenes: Bandwidth Requirements of HPC Applications With Asynchronous I/O," which was published at the CLUSTER 2024

Before you start, first set up the correct [TMIO version](#ftio-version).
The experiments are divided into four parts:

- [Artifacts Reproducibility](#artifacts-reproducibility)
	- [Prerequisites](#prerequisites)
		- [TMIO Version](#tmio-version)
		- [Extracting the Data Set:](#extracting-the-data-set)
	- [Artifacts](#artifacts)
		- [Modified HACC-IO](#modified-hacc-io)
	- [Citation](#citation)

## Prerequisites 
Before you start, there are two prerequisites:
1. Install the correct [FTIO version](#ftio-version) 
2. Depending on what you want to test, you need to [download and extract](#extracting-the-data-set) the data set from [Zenodo](https://doi.org/10.5281/zenodo.12700677).

### TMIO Version

For all the cases below, `tmio` first needs to be installed (see [Installation](https://github.com/tuda-parallel/TMIO?tab=readme-ov-file#installation)). We used `tmio` version 0.0.1 for the experiment in the paper. To get this version, simply execute the following code:
```sh
git checkout v0.0.1 
```

### Extracting the Data Set:
download the zip file from [here](https://doi.org/10.5281/zenodo.12700677) or using wget in a bash terminal:
```sh
wget https://zenodo.org/records/12700677/files/data.zip?download=1
```
Next, unzip the file
```sh
unzip data.zip
```
This extracts the needed traces and experiments:

```sh
data
└─── application_traces
    ├── WACOM++
    └── HACC-IO

```

## Artifacts

### Modified HACC-IO
Either extract the data set as described [here](#extracting-the-data-set) or download the modified HACC-IO code from [GitHub](https://github.com/A-Tarraf/hacc-io)




## Citation
The paper citation is also available [here](/README.md#citation). You can cite the [data set](https://doi.org/10.5281/zenodo.12700677) as:
```
 
 @inproceedings{AT24a, 
   author={Tarraf, Ahmad and Muñoz, Javier Fernandez and Singh, David E. and Özden, Taylan and Carretero, Jesus and Wolf, Felix},
   title={I/O Behind the Scenes: Bandwidth Requirements of HPC Applications With Asynchronous I/O}, 
   address={Kobe, Japan}, 
   booktitle={2024 IEEE International Conference on Cluster Computing (CLUSTER)}, 
   year={2024}, 
   month={sep},
   note={(accepted)}
 }


@dataset{tarraf_2024_12700678,
  author       = {Tarraf, Ahmad and
                  Muñoz, Javier Fernández and
                  Singh, David E. and
                  Özden, Taylan and
                  Carretero, Jesus and
                  Wolf, Felix},
  title        = {I/O Behind the Scenes [Data Set]},
  month        = jul,
  year         = 2024,
  publisher    = {Zenodo},
  Howpublished = {Zenodo},
  doi          = {10.5281/zenodo.12700677},
  url          = {https://doi.org/10.5281/zenodo.12700677}
}
```

