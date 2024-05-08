using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class spilHaandtere : MonoBehaviour
{
    OSCReceiver oscR;
    public GameObject klods;
    public GameObject farve;
    public GameObject knap;
    public GameObject wire;
    public GameObject wall;
    public GameObject sol;
    public GameObject lys;
    public GameObject red;
    public GameObject green;
    public GameObject blue;
    public float lysfarve;
    float intensivitet = 7.5f;
    public int antalBrikker = 0;
    public int count = 0;
    // Start is called before the first frame update
    void Start()
    {
        oscR = GameObject.Find("OSC_Control").GetComponent<OSCReceiver>();
        oscR.UnlockJoystick();
    }

    // Update is called once per frame
    void Update()
    {
        lys.GetComponent<Light>().intensity = intensivitet;
        if (intensivitet > 1)
        {
            intensivitet += -0.01f;
        }

        if (antalBrikker == 3)
        {
            knap.SetActive(true);
        }
        if (count > 0)
        {
            count += -1;
        }
        if (lysfarve < 10)
        {
            lys.GetComponent<Light>().color = Color.white;
            red.SetActive(false);
        }
        else if (lysfarve >= 10 && lysfarve < 30)
        {
            lys.GetComponent<Light>().color = Color.red;
            red.SetActive(true);
            green.SetActive(false);
        }
        else if (lysfarve >= 30 && lysfarve < 50)
        {
            lys.GetComponent<Light>().color = Color.green;
            red.SetActive(false);
            green.SetActive(true);
            blue.SetActive(false);
        }
        else if (lysfarve >= 50)
        {
            lys.GetComponent<Light>().color = Color.blue;
            green.SetActive(false);
            blue.SetActive(true);
        }
    }
    public void intensivitetReset()
    {
        Debug.Log("hej");
        intensivitet = 7.5f;
        Debug.Log("hej2");
    }

    public void klodsStart()
    {
        klods.SetActive(true);
        farve.SetActive(false);
    }

    public void passcodeDone ()
    {
        wire.SetActive(true);   
    }

    public void complete()
    {
        wall.SetActive(false);
        sol.SetActive(true);
    }
    public void button()
    {
        for(int i = 0; i < 1000; i++)
        {
            if (count > 0)
            {
                oscR.UnlockEncoder();
            }   
        }
    }

}
