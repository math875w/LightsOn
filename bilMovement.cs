using System;
using System.Collections;
using System.Collections.Generic;
using Unity.Burst.Intrinsics;
using Unity.Mathematics;
using Unity.VisualScripting;

using UnityEngine;
using UnityEngine.Rendering;


public class bilMovement : MonoBehaviour
{
    OSCReceiver oscR;
    public float speed = 1;
    public float inputX = 0;
    public float inputY = 0;

    public GameObject FarveKombination;

    float maxInput;
    float rot = 0;

    void Start()
    {

        oscR = GameObject.Find("OSC_Control").GetComponent<OSCReceiver>();
    }

    // Update is called once per frame
    void Update()
    {   
        maxInput = math.max(math.abs(inputX),math.abs(inputY));
 
        Vector3 movement = new Vector3(speed * maxInput, 0, 0);

        float xrot = 0;
        float yrot = 0;
        if(inputX > 3 || inputX < -3 || inputY > 3 || inputY < -3){
            if(inputX>0){
                xrot = 180;
            } else if(inputX<0) {
                xrot = 0;
            }

            if(inputY>0){
                yrot = 90; 
            } else if(inputY<0) {
                yrot = 270;
                if (xrot == 0){
                    xrot = 360;
                }
            }

            rot = (xrot * math.abs(inputX) + yrot * math.abs(inputY))/(math.abs(inputX)+math.abs(inputY));
            if (double.IsNaN(rot)){
                rot = 0;
            }

            Quaternion Rotation = Quaternion.Euler(0, math.abs(rot), 0);
            Debug.Log(rot);

            movement *= Time.deltaTime;
            transform.rotation = Rotation;
            transform.Translate(movement);
        }
    }
    private void OnTriggerEnter(Collider other) {

        if (other.CompareTag("mousegoal")) {
            oscR.UnlockKnap();
            Destroy(other);
            FarveKombination.SetActive(true);
        }
    }
}
