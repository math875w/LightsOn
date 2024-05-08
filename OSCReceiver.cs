using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using UnityEngine.UI;
using System.Net.Sockets;
using System.Net;
using System.Linq;
using UnityEngine.XR;
using Unity.VRTemplate;
using Unity.VisualScripting;
using UnityEngine.XR.Interaction.Toolkit.Inputs;
using System;

[AddComponentMenu("Scripts/OSCReceiver")]
public class OSCReceiver : MonoBehaviour
{
    public string remoteIp = "192.168.50.151";
    public int sendToPort = 8888;  // Sæt denne til 6448, hvis der sendes til wekinator
    public int listenerPort = 12000;
    //public GameObject spotOSC;
    //public GameObject lockOSC;
    //public Light sceneLight;
    private float xRot = 0; 
    private float xJoy = 0;
    private float yRot = 0;
    private float yJoy = 0;
    private float zRot = 0;

    private Osc oscHandler;
    
    public GameObject bil;
    public bilMovement bilMovement;
    public GameObject haandtere;
    public spilHaandtere spilHaandtere;

    public GameObject messageCanvas;
    public Text messageText;
    
    private string message;
    private float lightLevel = 8;
    public bool lightOn = false;
    
    
    ~OSCReceiver()
    {
        if (oscHandler != null)
        {            
            oscHandler.Cancel();
        }

        // speed up finalization
        oscHandler = null;
        System.GC.Collect();
    }

    /// <summary>
    /// Update is called every frame, if the MonoBehaviour is enabled.
    /// </summary>
    void Update()
    {
        bilMovement.inputX = xJoy;
        bilMovement.inputY = yJoy;
        if(messageText){
            messageText.text = message;
        }
        // spotOSC.GetComponent<Light>().intensity = lightLevel;
    	// spotOSC.transform.localEulerAngles = new Vector3(xRot, yRot, zRot);
        // sceneLight.enabled = lightOn;
    
    }

    /// <summary>
    /// Awake is called when the script instance is being loaded.
    /// </summary>
    void Awake()
    {
       
    }

    void OnDisable()
    {
        // close OSC UDP socket
        Debug.Log("closing OSC UDP socket in OnDisable");
        oscHandler.Cancel();
        oscHandler = null;
    }

    /// <summary>
    /// Start is called just before any of the Update methods is called the first time.
    /// </summary>
    void Start()
    {
        SetUpOSC();


        if (messageCanvas == null) {
            messageCanvas = transform.Find("OscMessageCanvas").gameObject;
            if (messageCanvas != null) {
                messageText = messageCanvas.transform.Find("MessageText").GetComponent<Text>();
            }
        }
        
        // if (spotOSC == null) {
        //     spotOSC = transform.Find("SpotLight").gameObject;  // SpotLight            
        // }

        //if (sceneLight == null) {
            //sceneLight = transform.Find("SceneLight").gameObject.GetComponent<Light>(); 
        //}
                 
        string localIP;
        using (Socket socket = new Socket(AddressFamily.InterNetwork, SocketType.Dgram, 0))
        {
            socket.Connect("8.8.8.8", 65530);
            IPEndPoint endPoint = socket.LocalEndPoint as IPEndPoint;
            localIP = "IP-adressen her er " + endPoint.Address.ToString();
            Debug.Log(localIP);
        }

        List<InputDevice> dimser = new List<InputDevice>();
        string dimseNavne = "";
        InputDevices.GetDevices(dimser);
        foreach (InputDevice d in dimser) {
            dimseNavne += "\n";
            dimseNavne += d.characteristics;
        }
        SetText(localIP + dimseNavne);
        OscMessage oscM = Osc.StringToOscMessage(localIP);
        oscHandler.Send(oscM);
    }

    void SetUpOSC() {
        UDPPacketIO udp = GetComponent<UDPPacketIO>();
        udp.init(remoteIp, sendToPort, listenerPort);
        
	    oscHandler = GetComponent<Osc>();
        oscHandler.init(udp);

        oscHandler.SetAddressHandler("/remoteIP", SetRemoteIP);        
        oscHandler.SetAddressHandler("/joystick", Joystick);
        oscHandler.SetAddressHandler("/encoder", Encoder);
        oscHandler.SetAddressHandler("/knap", Knap);
        oscHandler.SetAddressHandler("/farve", Farve);
        oscHandler.SetAddressHandler("/passcode", Passcode);
        oscHandler.SetAddressHandler("/lys", Lys);
        oscHandler.SetAddressHandler("/aktiver", Find);
        oscHandler.SetAddressHandler("/text", TextFromOSC);
        oscHandler.SetAddressHandler("/light/direction", LightFromOSC);
        oscHandler.SetAddressHandler("/light/intensity", LightFromOSC);
        oscHandler.SetAddressHandler("/lock", LockFromOSC);
        SetText("OSC sat op til remoteIP " + remoteIp);
    }
    
    public void SendOscMessage(string address, string data) {
        OscMessage oscM = Osc.StringToOscMessage(address + " " + data);
        oscHandler.Send(oscM);
    }
    
    public void SetText(string str){
        message = str;
    }



    public void SetRemoteIP(OscMessage m) {
        Debug.Log("Called SetRemoteIP from OSC >> " + Osc.OscMessageToString(m));
        SetText("Skifter remoteIP " + Osc.OscMessageToString(m));
        
        remoteIp = (string) m.Values[0];
        
        oscHandler.Cancel();
        oscHandler = null;

        SetUpOSC();

    }

    public void LightFromOSC(OscMessage m) {
        
        Debug.Log("Called light from OSC >> " + Osc.OscMessageToString(m));
        //setText((string) m.Values[0]);
        string[] addressParts = m.Address.Split('/');
        Debug.Log("   Address Last Parts: " + addressParts[addressParts.Length-1]);
        if(addressParts[addressParts.Length-1] == "intensity") {
            lightLevel = (float) (((int) m.Values[0]) * 0.01);
        }
        if (addressParts[addressParts.Length-1] == "direction") {
            xRot = (float) (((int) m.Values[0]) * 0.01);
            yRot = (float) (((int) m.Values[1]) * 0.01);
            zRot = (float) (((int) m.Values[2]) * 0.01);
        }
    }
    
    public void TextFromOSC(OscMessage m)
    {
        Debug.Log("Called text from OSC > " + Osc.OscMessageToString(m));
        SetText((string) m.Values[0]);
    }

    public void LockFromOSC(OscMessage m) {
        SetText(Osc.OscMessageToString(m));
        lightOn = (int) m.Values[0] > 0;
    }
    public void Farve(OscMessage m) {
        if(m.ToString() == "01101"){
            haandtere.GetComponent<spilHaandtere>().klodsStart();
        }
    }
    public void Passcode(OscMessage m) {
        if(m.ToString() == "238155"){
            OscMessage oscM = Osc.StringToOscMessage("/doorOpen");
            oscHandler.Send(oscM);
            haandtere.GetComponent<spilHaandtere>().passcodeDone();

        }
    }
    public void Joystick(OscMessage m) {
        
        Debug.Log("Called light from OSC >> " + Osc.OscMessageToString(m));
        //setText((string) m.Values[0]);
        string[] addressParts = m.Address.Split('/');
        Debug.Log("   Address Last Parts: " + addressParts[addressParts.Length-1]);
        // xRot = (float) m.Values[0];
        xJoy = (float.Parse(m.Values[0].ToString()) - 500) * 0.01f;
        yJoy = (float.Parse(m.Values[1].ToString()) - 500) * 0.01f;
        // Debug.Log("xJoy: " + xJoy + ", " + Int32.Parse(m.Values[0]));
    }
    public void Encoder(OscMessage m) {
        haandtere.GetComponent<spilHaandtere>().lysfarve = Mathf.Abs(float.Parse(m.ToString()));
    }
    public void Lys(OscMessage m) {
        Debug.Log("lys1");
        try {
            spilHaandtere.intensivitetReset();
            Debug.Log("lys2");
        } catch (Exception ex) {
            Debug.LogError("Error in Lys method: " + ex.Message);
        }
    }

    public void Knap(OscMessage m) {
        haandtere.GetComponent<spilHaandtere>().count = 1000;
    }

    public void Find(OscMessage m)
    {
        Debug.Log("Finder > " + m.Address);
        //Find()
        string msg = Osc.OscMessageToString(m);
    	Debug.Log("Fandt > " + msg);
        SetText("Aktiveret " + msg);
    }

    public void SetGazedAt(bool gazedAt)
    {
        Debug.Log("Gazed at!");
        SetText("Den er set!!");
    }

    public void QuestDone() {
        //SetText("Super Duper, Færdigt arbejde");
        OscMessage oscM = Osc.StringToOscMessage("/qd");
        oscHandler.Send(oscM);
    }

    public void UnlockEncoder() {
        OscMessage oscM = Osc.StringToOscMessage("/encoderUnlock");
        oscHandler.Send(oscM);
    }
    public void UnlockJoystick() {
        OscMessage oscM = Osc.StringToOscMessage("/joystickUnlock");
        oscHandler.Send(oscM);
    }
    public void UnlockKnap() {
        OscMessage oscM = Osc.StringToOscMessage("/knapUnlock");
        oscHandler.Send(oscM);
    }
    public void UnlockDisplay() {
        OscMessage oscM = Osc.StringToOscMessage("/displayUnlock");
        oscHandler.Send(oscM);
    }

    public void KeyReady() {
        lightOn = true;
        //lockOSC.SetActive(true);
        SetText("Flot - du har nøglen, læg den på låsekassen!");
    }

}