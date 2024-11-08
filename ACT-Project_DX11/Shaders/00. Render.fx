#ifndef _RENDER_FX_
#define _RENDER_FX_

#include "00. Global.fx"

#define MAX_MODEL_TRANSFORMS 250
#define MAX_MODEL_KEYFRAMES 500
#define MAX_MODEL_INSTANCE 500

// ************** InstancingMeshRender ****************

struct InstancingVertexMesh
{
	float4 position : POSITION;
	float2 uv : TEXCOORD;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	// INSTANCING;
	uint instanceID : SV_INSTANCEID;
	matrix world : INST;
};

MeshOutput VS_InstancingMesh(InstancingVertexMesh input)
{
	MeshOutput output;

	output.position = mul(input.position, input.world); // W
	output.worldPosition = output.position;
	output.position = mul(output.position, VP);
	output.uv = input.uv;
	output.normal = input.normal;

	return output;
}

// ************** SingleMeshRender ****************

MeshOutput VS_Mesh(VertexTextureNormal input)
{
    MeshOutput output;

    output.position = mul(input.position, W); // W
    output.worldPosition = output.position;
    output.position = mul(output.position, VP);
    output.uv = input.uv;
    output.normal = input.normal;

    return output;
}

MeshOutput VS_MeshColor(VertexColor input)
{
    MeshOutput output;
    output.position = input.position;

    return output;
}

// ************** InstancingModelRender ****************

struct InstancingVertexModel
{
	float4 position : POSITION;
	float2 uv : TEXCOORD;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float4 blendIndices : BLEND_INDICES;
	float4 blendWeights : BLEND_WEIGHTS;
	// INSTANCING;
	uint instanceID : SV_INSTANCEID;
	matrix world : INST;
};

cbuffer BoneBuffer
{
	matrix BoneTransforms[MAX_MODEL_TRANSFORMS];
};

uint BoneIndex;

MeshOutput VS_InstancingModel(InstancingVertexModel input)
{
	MeshOutput output;

	output.position = mul(input.position, BoneTransforms[BoneIndex]); // Model Global
	output.position = mul(output.position, input.world); // W
	output.worldPosition = output.position;
	output.position = mul(output.position, VP);
	output.uv = input.uv;
	output.normal = input.normal;

	return output;
}

// ************** SingleModelRender ****************
MeshOutput VS_Model(VertexTextureNormalTangent input)
{
    MeshOutput output;

    output.position = mul(input.position, BoneTransforms[BoneIndex]); // Model Global
    output.position = mul(output.position, W); // W
    output.worldPosition = output.position;
    output.position = mul(output.position, VP);
    output.uv = input.uv;
    output.normal = input.normal;

    return output;
}

// ************** InstancingAnimRender ****************

struct KeyframeDesc
{
    int state;			// 4바이트: 애니메이션 상태
    int animIndex;		// 4바이트: 애니메이션 인덱스
    uint currFrame;		// 4바이트: 현재 프레임
    uint nextFrame;		// 4바이트: 다음 프레임
    float ratio;		// 4바이트: 현재 프레임과 다음 프레임 사이의 보간 비율
    float sumTime;		// 4바이트: 애니메이션 진행 시간 합계
    float speed;		// 4바이트: 애니메이션 재생 속도
    float padding;		// 4바이트: 32바이트로 맞추기 위한 패딩
};

struct TweenFrameDesc
{
	float tweenDuration;
	float tweenRatio;
	float tweenSumTime;
	float padding;
	KeyframeDesc curr;
	KeyframeDesc next;
};

cbuffer InstancedTweenBuffer
{
	TweenFrameDesc InstancedTweenFrames[MAX_MODEL_INSTANCE];
};

Texture2DArray TransformMap;

matrix GetAnimationMatrix(InstancingVertexModel input)
{
	float indices[4] = { input.blendIndices.x, input.blendIndices.y, input.blendIndices.z, input.blendIndices.w };
	float weights[4] = { input.blendWeights.x, input.blendWeights.y, input.blendWeights.z, input.blendWeights.w };
	
	// 현재 애니메이션 상태에 대한 정보
	int animIndex[2];
	int currFrame[2];
	int nextFrame[2];
	float ratio[2];

    animIndex[0] = InstancedTweenFrames[input.instanceID].curr.animIndex;
    currFrame[0] = InstancedTweenFrames[input.instanceID].curr.currFrame;
    nextFrame[0] = InstancedTweenFrames[input.instanceID].curr.nextFrame;
    ratio[0] = InstancedTweenFrames[input.instanceID].curr.ratio;

    animIndex[1] = InstancedTweenFrames[input.instanceID].next.animIndex;
    currFrame[1] = InstancedTweenFrames[input.instanceID].next.currFrame;
    nextFrame[1] = InstancedTweenFrames[input.instanceID].next.nextFrame;
    ratio[1] = InstancedTweenFrames[input.instanceID].next.ratio;
	
    float4 c0, c1, c2, c3; // 현재 프레임의 변환 컴포넌트
    float4 n0, n1, n2, n3; // 다음 프레임의 변환 컴포넌트
    matrix curr = 0; // 현재 프레임의 변환 행렬
    matrix next = 0; // 다음 프레임의 변환 행렬
    matrix transform = 0; // 최종 계산된 변환 행렬

	// 각 본에 대해 변환 행렬을 계산
	for (int i = 0; i < 4; i++)
	{
		// 현재 프레임의 변환 컴포넌트 로드
		c0 = TransformMap.Load(int4(indices[i] * 4 + 0, currFrame[0], animIndex[0], 0));
		c1 = TransformMap.Load(int4(indices[i] * 4 + 1, currFrame[0], animIndex[0], 0));
		c2 = TransformMap.Load(int4(indices[i] * 4 + 2, currFrame[0], animIndex[0], 0));
		c3 = TransformMap.Load(int4(indices[i] * 4 + 3, currFrame[0], animIndex[0], 0));
		curr = matrix(c0, c1, c2, c3);	// 현재 프레임의 변환 행렬 생성

		// 다음 프레임의 변환 컴포넌트 로드
		n0 = TransformMap.Load(int4(indices[i] * 4 + 0, nextFrame[0], animIndex[0], 0));
		n1 = TransformMap.Load(int4(indices[i] * 4 + 1, nextFrame[0], animIndex[0], 0));
		n2 = TransformMap.Load(int4(indices[i] * 4 + 2, nextFrame[0], animIndex[0], 0));
		n3 = TransformMap.Load(int4(indices[i] * 4 + 3, nextFrame[0], animIndex[0], 0));
		next = matrix(n0, n1, n2, n3); // 다음 프레임의 변환 행렬 생성

		matrix result = lerp(curr, next, ratio[0]); // 현재와 다음 프레임 사이 보간

		// 다음 애니메이션
		if (animIndex[1] >= 0)
		{
			c0 = TransformMap.Load(int4(indices[i] * 4 + 0, currFrame[1], animIndex[1], 0));
			c1 = TransformMap.Load(int4(indices[i] * 4 + 1, currFrame[1], animIndex[1], 0));
			c2 = TransformMap.Load(int4(indices[i] * 4 + 2, currFrame[1], animIndex[1], 0));
			c3 = TransformMap.Load(int4(indices[i] * 4 + 3, currFrame[1], animIndex[1], 0));
			curr = matrix(c0, c1, c2, c3);

			n0 = TransformMap.Load(int4(indices[i] * 4 + 0, nextFrame[1], animIndex[1], 0));
			n1 = TransformMap.Load(int4(indices[i] * 4 + 1, nextFrame[1], animIndex[1], 0));
			n2 = TransformMap.Load(int4(indices[i] * 4 + 2, nextFrame[1], animIndex[1], 0));
			n3 = TransformMap.Load(int4(indices[i] * 4 + 3, nextFrame[1], animIndex[1], 0));
			next = matrix(n0, n1, n2, n3);

			matrix nextResult = lerp(curr, next, ratio[1]);
            result = lerp(result, nextResult, InstancedTweenFrames[input.instanceID].tweenRatio);
        }

		transform += mul(weights[i], result); // 가중치 적용하여 변환 행렬 누적
	}

	return transform; // 최종 변환 행렬 반환
}

MeshOutput VS_InstancingAnimation(InstancingVertexModel input)
{
	MeshOutput output;

	//output.position = mul(input.position, BoneTransforms[BoneIndex]); // Model Global

	matrix m = GetAnimationMatrix(input);	// 애니메이션 변환 행렬 계산

	output.position = mul(input.position, m);	// 정점 위치 변환
	output.position = mul(output.position, input.world); // 월드 변환 적용
	output.worldPosition = output.position;	// 월드 좌표 설정
	output.position = mul(output.position, VP);	// 뷰-프로젝션 변환 적용
	output.uv = input.uv;	// UV 좌표는 변경 없음
	output.normal = mul(input.normal, (float3x3)input.world);	// 법선 벡터 변환
	output.tangent = mul(input.tangent, (float3x3)input.world);	// 탄젠트 벡터 변환

	return output;	// 변환된 정점 데이트 반환
}

// ************** SingleAnimRender ****************

cbuffer TweenBuffer
{
    TweenFrameDesc SingleTweenFrames;
};

matrix GetAnimationMatrix(VertexTextureNormalTangentBlend input)
{
    float indices[4] = { input.blendIndices.x, input.blendIndices.y, input.blendIndices.z, input.blendIndices.w };
    float weights[4] = { input.blendWeights.x, input.blendWeights.y, input.blendWeights.z, input.blendWeights.w };

    int animIndex[2];
    int currFrame[2];
    int nextFrame[2];
    float ratio[2];

    animIndex[0] = SingleTweenFrames.curr.animIndex;
    currFrame[0] = SingleTweenFrames.curr.currFrame;
    nextFrame[0] = SingleTweenFrames.curr.nextFrame;
    ratio[0] = SingleTweenFrames.curr.ratio;

    animIndex[1] = SingleTweenFrames.next.animIndex;
    currFrame[1] = SingleTweenFrames.next.currFrame;
    nextFrame[1] = SingleTweenFrames.next.nextFrame;
    ratio[1] = SingleTweenFrames.next.ratio;

    float4 c0, c1, c2, c3;
    float4 n0, n1, n2, n3;
    matrix curr = 0;
    matrix next = 0;
    matrix transform = 0;

    for (int i = 0; i < 4; i++)
    {
        c0 = TransformMap.Load(int4(indices[i] * 4 + 0, currFrame[0], animIndex[0], 0));
        c1 = TransformMap.Load(int4(indices[i] * 4 + 1, currFrame[0], animIndex[0], 0));
        c2 = TransformMap.Load(int4(indices[i] * 4 + 2, currFrame[0], animIndex[0], 0));
        c3 = TransformMap.Load(int4(indices[i] * 4 + 3, currFrame[0], animIndex[0], 0));
        curr = matrix(c0, c1, c2, c3);

        n0 = TransformMap.Load(int4(indices[i] * 4 + 0, nextFrame[0], animIndex[0], 0));
        n1 = TransformMap.Load(int4(indices[i] * 4 + 1, nextFrame[0], animIndex[0], 0));
        n2 = TransformMap.Load(int4(indices[i] * 4 + 2, nextFrame[0], animIndex[0], 0));
        n3 = TransformMap.Load(int4(indices[i] * 4 + 3, nextFrame[0], animIndex[0], 0));
        next = matrix(n0, n1, n2, n3);

        matrix result = lerp(curr, next, ratio[0]);

		// 다음 애니메이션
        if (animIndex[1] >= 0)
        {
            c0 = TransformMap.Load(int4(indices[i] * 4 + 0, currFrame[1], animIndex[1], 0));
            c1 = TransformMap.Load(int4(indices[i] * 4 + 1, currFrame[1], animIndex[1], 0));
            c2 = TransformMap.Load(int4(indices[i] * 4 + 2, currFrame[1], animIndex[1], 0));
            c3 = TransformMap.Load(int4(indices[i] * 4 + 3, currFrame[1], animIndex[1], 0));
            curr = matrix(c0, c1, c2, c3);

            n0 = TransformMap.Load(int4(indices[i] * 4 + 0, nextFrame[1], animIndex[1], 0));
            n1 = TransformMap.Load(int4(indices[i] * 4 + 1, nextFrame[1], animIndex[1], 0));
            n2 = TransformMap.Load(int4(indices[i] * 4 + 2, nextFrame[1], animIndex[1], 0));
            n3 = TransformMap.Load(int4(indices[i] * 4 + 3, nextFrame[1], animIndex[1], 0));
            next = matrix(n0, n1, n2, n3);

            matrix nextResult = lerp(curr, next, ratio[1]);
            result = lerp(result, nextResult, SingleTweenFrames.tweenRatio);
        }

        transform += mul(weights[i], result);
    }

    return transform;
}

MeshOutput VS_Animation(VertexTextureNormalTangentBlend input)
{
    MeshOutput output;

	//output.position = mul(input.position, BoneTransforms[BoneIndex]); // Model Global

	matrix m = GetAnimationMatrix(input);

	output.position = mul(input.position, m);
	output.position = mul(output.position, W); // W
	output.worldPosition = output.position;
	output.position = mul(output.position, VP);
	output.uv = input.uv;
	output.normal = mul(input.normal, (float3x3)W);
	output.tangent = mul(input.tangent, (float3x3)W);

	return output;
}

#endif

