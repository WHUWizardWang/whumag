attribute vec4 vertex;
attribute vec3 normal;
attribute vec3 colors;
attribute float type;

varying vec3 vert;
varying vec3 color;
varying vec3 vertNormal;
//varying float colorIndex;

uniform mat4 projMatrix;
uniform mat4 mvMatrix;
uniform mat3 normalMatrix;
uniform float scale;
//uniform int colorNum;

void main() {
//   vert = vertex.xyz;   //vec3(vertex.xy, vertex.z*scale);
   if(type<0.5){
       vert = vec3(vertex.xy, vertex.z*scale);
   } else {
       vert = vertex.xyz;
   }
   color = colors;
//   colorIndex = (vert.z+0.5*scale)/scale*float(colorNum - 1);
   vertNormal = normalMatrix * normal;
   gl_Position = projMatrix * mvMatrix * vec4(vert,vertex.w);
}
