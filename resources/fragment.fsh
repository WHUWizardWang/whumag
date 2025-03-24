varying highp vec3 vert;
varying highp vec3 color;
varying highp vec3 vertNormal;
//varying highp float colorIndex;
uniform highp vec3 lightPos;
//uniform highp vec3 colorMap[256];
uniform int closeShade;
uniform int closeTexture;
void main() {
   highp vec3 L = normalize(lightPos - vert);
   highp float NL = max(dot(normalize(vertNormal), L), 0.0);

   // Texture
   highp vec3 ptclr = color;
//   if(closeTexture>0){
//       ptclr = colorMap[int(colorIndex)];
//   }

   // Light
   highp vec3 col = clamp(ptclr * 0.2 + ptclr * 0.8 * NL, 0.0, 1.0);
   if (closeShade > 0){
       col = ptclr;
   }

   gl_FragColor = vec4(col, 1.0);
}
