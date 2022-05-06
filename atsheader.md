# atss format description

The atss JSON is a sparse header - containing as less information as possible. This makes the *re-locatable*.
If the site name would be included all headers must be edited.
Instead you may use a *directory* as a natural container for the site data.

## Directory Tree

```
Northern_Mining
├── cal
│   └── sensors.db
├── config
│   └── autogen_config.xml
├── db
│   └── local.db
├── doc
├── dump
│   └── Ex.spcdata
├── edi
│   ├── Sarıçam_1.edi
│   ├── Sarıçam_2.edi
│   └── Sarıçam.edi
├── filters
├── jle
├── jobs
├── log
├── processings
│   └── mt_auto_ct.xml
├── shell
│   └── procall.sh
├── tmp
└── ts
    └── Sarıçam
        ├── meas_2009-08-20_13-22-00
        └── meas_2009-08-20_13-22-01
```
