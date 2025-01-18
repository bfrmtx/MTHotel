import base64
import os
import mimetypes
from xml.dom import minidom


def embed_images(svg_file, svg_file_out=None):
    doc = minidom.parse(svg_file)
    images = doc.getElementsByTagName("image")
    for img in images:
        if img.hasAttribute("xlink:href"):
            resource_file = img.getAttribute("xlink:href")
            if os.path.isfile(resource_file):
                mime_type = [mime for mime in mimetypes.guess_type(resource_file) if mime != None]
                with open(resource_file, "rb") as image_file:
                    encoded = base64.b64encode(image_file.read()).decode()
                    attr = f"data:{mime_type};base64," + encoded
                    img.setAttribute("xlink:href", attr)

    # xmlns:xlink="http://www.w3.org/1999/xlink"
    # so remove the string above from the svg file
    # https://stackoverflow.com/questions/1091945/strip-xlink-namespace-from-svg-with-python
    for node in doc.getElementsByTagName("svg"):
        if node.hasAttribute("xmlns:xlink"):
            node.removeAttribute("xmlns:xlink")
    # also change all <image xlink:href to <image href
    for node in doc.getElementsByTagName("image"):
        if node.hasAttribute("xlink:href"):
            node.setAttribute("href", node.getAttribute("xlink:href"))
            node.removeAttribute("xlink:href")

    if not svg_file_out:
        p, ext = os.path.splitext(svg_file)
        svg_file_out = p + "_embedded" + ext
    with open(svg_file_out, "w") as f:
        f.write(doc.toxml())
    # return svg_file_out
    return svg_file_out

    
