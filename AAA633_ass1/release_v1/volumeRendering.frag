#version 330 core

in vec3 pixelPosition;

uniform sampler3D tex;
uniform sampler1D transferFunction;
uniform int method;
uniform float isoValue;
uniform float xrot;
uniform float yrot;
uniform float zoom;


vec3 eyePosition = vec3(0.0, 0.0, 50.0);
vec3 lightPosition = vec3(0.0, 3.0, -3.0);
vec3 lightAmb = vec3(0.4);
vec3 lightDif = vec3(0.7);
vec3 lightSpec = vec3(0.6);
float shininess = 10.0f;
float stepSize = 0.001;

struct Ray {
	vec3 Ori;
	vec3 Dir;
};

struct AABBbox {
	vec3 Min;
	vec3 Max;
};


bool intersectBox(Ray r, AABBbox aabb, out float tNear, out float tFar)
{
	vec3 tMin = (aabb.Min-r.Ori) / r.Dir;
    vec3 tMax = (aabb.Max-r.Ori) / r.Dir;
    vec3 t1 = min(tMin, tMax);
    vec3 t2 = max(tMin, tMax);
    tNear = max(max(t1.x, t1.y), t1.z);
    tFar = min(min(t2.x, t2.y), t2.z);
    if(tFar <= tNear)
	{
		return false;
	}
	return true;
}

vec3 erot(vec3 p, vec3 ax, float ro)
{
	return mix(dot(ax, p)*ax, p, cos(ro)) + cross(ax, p)*sin(ro);
}

//MIP
void MIP(vec3 entry, vec3 exit)
{
	vec3 Dir = exit - entry;
	float stepLength = 0.0;
	float intensity = 0.0;
	float maxIntensity = 0.0;

	while(length(Dir) > stepLength)
	{
		intensity = texture(tex, entry + (normalize(Dir) * stepLength)).r;
		if(intensity > maxIntensity)
		{
			maxIntensity = intensity;
		}

		stepLength += stepSize;
	}
	gl_FragColor = texture(transferFunction, maxIntensity);
}

//Alpha compositing
void AlphaCompositing(vec3 entry, vec3 exit)
{
	vec3 Dir = exit - entry;
	vec4 colorSample = vec4(0.0);
	vec4 colorAccum = vec4(0.0);
	float voxelValue = 0.0;
	float stepLength = 0.0;
	float x = 7.0;
	float alpha = 0.0;

	while(length(Dir) > stepLength && colorSample.a < 1.f)
	{
		voxelValue = texture(tex, entry + (normalize(Dir) * stepLength)).r;
		colorSample = texture(transferFunction, voxelValue);
 
		alpha = pow(colorSample.a, x);

		//front-to-back compositing
		colorAccum.rgb += (1.0 - colorAccum.a) * colorSample.rgb * alpha;
		colorAccum.a += (1.0 - colorAccum.a) * alpha;	
		
		stepLength += stepSize;
	}
	gl_FragColor = colorAccum;	
}

//Iso-surface rendering using Phong illumination
void IsosurfacePhong(vec3 entry, vec3 exit)
{
	vec3 pos = entry;
	vec3 Dir = exit - entry;
	vec3 zeroPos = vec3(0.0);
	float stepLength = 0.0;
	float posValue1, posValue2;
	float x = 0.01;

	vec3 ambient, diffuse, specular;
	vec3 gradient = vec3(0.0);
	vec3 normal = vec3(0.0);
	vec3 lightDir = vec3(0.0);
	vec3 viewDir = vec3(0.0);
	vec3 reflectDir = vec3(0.0);
	vec4 zeroColor = vec4(0.0);

	while(length(Dir) > stepLength)
	{
		posValue1 = texture(tex, pos).r;
		posValue2 = texture(tex, pos + (normalize(Dir) * stepSize)).r;
		if((posValue1 <= isoValue && posValue2 > isoValue) || (posValue1 >= isoValue && posValue2 < isoValue))
		{
			zeroPos = pos;
			// while(abs(isoValue - texture(tex, zeroPos).r) > x)
			// {
			// 	zeroPos += normalize(Dir) * (stepSize / 2);
			// }		

			zeroColor = texture(transferFunction, texture(tex, zeroPos).r);
			
			gradient.x = texture(tex, vec3(zeroPos.x + stepSize, zeroPos.yz)).r - texture(tex, vec3(zeroPos.x - stepSize, zeroPos.yz)).r;
			gradient.y = texture(tex, vec3(zeroPos.x, zeroPos.y + stepSize, zeroPos.z)).r - texture(tex, vec3(zeroPos.x, zeroPos.y - stepSize, zeroPos.z)).r;
			gradient.z = texture(tex, vec3(zeroPos.xy, zeroPos.z + stepSize)).r - texture(tex, vec3(zeroPos.xy, zeroPos.z - stepSize)).r;
			normal = normalize(gradient);
			lightDir = normalize(lightPosition - zeroPos);

			ambient = lightAmb * zeroColor.rgb;
			diffuse = max(dot(normal, lightDir), 0.0f) * lightDif * zeroColor.rgb;
			viewDir = normalize(eyePosition - zeroPos);
			reflectDir = reflect(-lightDir, normal);
			specular = pow(max(dot(viewDir, reflectDir), 0.0f), shininess) * lightSpec;

			gl_FragColor = vec4(vec3(ambient + diffuse + specular), zeroColor.a);

		}
		pos += normalize(Dir) * stepSize;
		stepLength += stepSize;
	}
}


void main()
{
	
	Ray eye = Ray(eyePosition, normalize(pixelPosition - eyePosition));

	eye.Ori = erot(eye.Ori, vec3(1,0,0), xrot);
	eye.Ori = erot(eye.Ori, vec3(0,1,0), yrot);

	eye.Ori *= zoom;
	
	eye.Dir = erot(eye.Dir, vec3(1,0,0), xrot);
	eye.Dir = erot(eye.Dir, vec3(0,1,0), yrot);
	
	AABBbox aabb = AABBbox(vec3(-1.0), vec3(1.0));

	float tnear, tfar;
	vec3 rayEntry, rayExit;
	if(intersectBox(eye, aabb, tnear, tfar))
	{
		rayEntry = eye.Ori + eye.Dir * tnear;
		rayExit = eye.Ori + eye.Dir * tfar;
		
		rayEntry += vec3(1.0,1.0,1.0);
		rayEntry /=2.0;
		rayExit += vec3(1.0,1.0,1.0);
		rayExit /=2.0;

		if(method == 0)
		{
			MIP(rayEntry, rayExit);
		}
		else if(method == 1)
		{
			AlphaCompositing(rayEntry, rayExit);
		}
		else if(method == 2)
		{
			IsosurfacePhong(rayEntry, rayExit);
		}		
	}
	
}