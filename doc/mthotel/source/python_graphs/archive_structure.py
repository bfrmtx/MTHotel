from diagrams import Diagram, Cluster, Edge
from diagrams.aws.compute import ECS
from diagrams.aws.database import RDS
from diagrams.aws.database import RedshiftDenseStorageNode
from pathlib import Path
import os

# full path to the directory where this script is located
script_dir = Path(__file__).parent
script_name = Path(__file__).name
title = os.path.splitext(script_name)[0]
out_file = os.path.join(script_dir, script_name)
# append a dot and the file extension to the script name
out_format = "svg"
out_file = os.path.splitext(out_file)[0]
# print(f"The output file is: {out_file}")

# inline = True

with Diagram("", show=False, outformat=out_format, filename=out_file ) as diag:

    with Cluster("Input"):
        # is the ordering here a bug? A appears on top of B and C
        # reverse order using slicing(::-1)
        #for item in a[::-1]:
            #print(item)
        # for the capital letters of the alphabet
        
        svc_group = array = []
        for i in range(2, -1, -1):
            svc_group.append(ECS(f"system type {chr(65+i)}"))
            
        ## pipe into service group
        
        #svc_group = [ECS("system type D"),
        #            ECS("system type C"),
        #            ECS("system type B"),
        #            ECS("system type A")]

    with Cluster("MTH5 Storage"):
        db_primary = RDS("MTH5")
        db_primary - Edge(color="red") - RedshiftDenseStorageNode("DUMP FS", color="blue")

    svc_group >> db_primary




try:
    import embed_images
    embed_images.embed_images(out_file + "." + out_format)
except ImportError:
    print("embed_images module is not available.")